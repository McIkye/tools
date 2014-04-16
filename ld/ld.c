/*
 * Copyright (c) 2009-2014 Michael Shalayeff
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef lint
static const char rcsid[] =
    "$ABSD: ld.c,v 1.41 2013/10/24 09:46:41 mickey Exp $";
#endif

#include <sys/param.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <elf_abi.h>
#include <elfuncs.h>
#include <a.out.h>
#include <ar.h>
#include <ranlib.h>
#include <getopt.h>
#include <paths.h>
#include <errno.h>
#include <err.h>

#include "ld.h"

TAILQ_HEAD(, pathlist) libdirs = TAILQ_HEAD_INITIALIZER(libdirs);
TAILQ_HEAD(objhead, objlist) objlist = TAILQ_HEAD_INITIALIZER(objlist);

/*
 * this object constain all the generated symbols and sections;
 */
struct objlist sysobj;

int endian;	/* ELFDATANONE */
int elfclass;	/* ELFCLASSNONE */
int machine;	/* EM_NONE */
int magic = ZMAGIC;
int as_needed;
int dflag;	/* force common allocation (even for -r) */
int Bflag;	/* 0 - static, 1 - dynamic, 2 - shlib */
int Xflag;	/* 0 - keep, 1 - sieve temps, 2 - sieve all locals */
int check_sections;
int gc_sections;
int export_dynamic;
int cref;
int nostdlib;
int pie;
int warncomm;
int randomise = 1;/* combine objects in random order (vs command line) */
int trace;	/* trace objects being loaded */
int relocatable;/* produce relocatable output */
int strip;
int errors;	/* non-fatal errors accumulated */
int printmap;	/* print edit map to stdout */
int eh_frame_hdr;
u_int64_t start_text, start_data, start_bss;
char *mapfile;
const char *entry_name;
struct symlist *sentry;

#define OPTSTRING "+A:B:c:C:d:D:e:Ef:F:gh:il:L:m:M:nNo:OqrR:sStT:u:vVxXy:Y:z:Z"
const struct option longopts[] = {
	{ "architecture",	required_argument,	0, 'A' },
	{ "as-needed",		no_argument,	&as_needed, 1 },
	{ "no-as-needed",	no_argument,	&as_needed, 0 },
	{ "check-sections",	no_argument,	&check_sections, 1 },
	{ "no-check-sections",	no_argument,	&check_sections, 0 },
	{ "cref",		no_argument,	&cref, 1 },
	{ "defsym",		required_argument,	0, 'D' },
	{ "nostdlib",		no_argument,	&nostdlib, 1 },
	{ "pie",		no_argument,	&pie, 1 },
	{ "pic-executable",	no_argument,	&pie, 1 },
/*	{ "rpath",		required_argument,	0, '???' }, */
	{ "section-start",	required_argument,	0, 'T' },
	{ "warn-common",	no_argument,	&warncomm, 1 },
	{ "shared",		no_argument,	&Bflag, 2 },
	{ "static",		no_argument,	&Bflag, 0 },
	{ "dc",			no_argument,		0, 'd' },
	{ "dn",			no_argument,	&Bflag, 0 },
	{ "dp",			no_argument,		0, 'd' },
	{ "dy",			no_argument,	&Bflag, 1 },
	{ "dynamic-linker",	required_argument,	NULL, 'd' },
	{ "entry",		required_argument,	0, 'e' },
	{ "export-dynamic",	no_argument,		0, 'E' },
	{ "eh-frame-hdr",	no_argument,	&eh_frame_hdr, 1 },
	{ "EB",			no_argument,	&endian, ELFDATA2MSB },
	{ "EL",			no_argument,	&endian, ELFDATA2LSB },
	{ "auxiliary",		required_argument,	0, 'f' },
	{ "filter",		required_argument,	0, 'F' },
	{ "fini",		required_argument,	0, 'c' },
	{ "gc-sections",	no_argument,	&gc_sections, 1 },
	{ "no-gc-sections",	no_argument,	&gc_sections, 0 },
	{ "soname",		required_argument,	0, 'h' },
	{ "init",		required_argument,	0, 'C' },
	{ "library",		required_argument,	0, 'l' },
	{ "library-path",	required_argument,	0, 'L' },
	{ "print-map",		no_argument,		0, 'M' },
	{ "Map",		required_argument,	0, 'M' },
	{ "no-random",		no_argument,	&randomise, 0 },
	{ "nmagic",		no_argument,	&magic, NMAGIC },
	{ "omagic",		no_argument,	&magic, OMAGIC },
	{ "output",		required_argument,	0, 'o' },
	{ "emit-relocs",	no_argument,		0, 'q' },
	{ "relocatable",	no_argument,		0, 'r' },
	{ "just-symbols",	required_argument,	0, 'R' },
	{ "strip-all",		no_argument,		0, 's' },
	{ "strip-debug",	no_argument,		0, 'S' },
	{ "trace",		no_argument,		0, 't' },
	{ "script",		required_argument,	0, 'T' },
	{ "undefined",		required_argument,	0, 'u' },
	{ "version",		no_argument,		0, 'v' },
	{ "discard-all",	no_argument,		0, 'x' },
	{ "discard-locals",	no_argument,		0, 'X' },
	{ "trace-symbol",	required_argument,	0, 'y' },
	{ NULL }
};

const struct ldarch ldarchs[] = {
/*	{ EM_VAX,	ELFCLASS32, vax_order, vax_fix }, */
/*	{ EM_ALPHA,	ELFCLASS64, alpha_order, alpha_fix }, */
	{ EM_386,	ELFCLASS32, i386_order, i386_fix },
	{ EM_AMD64,	ELFCLASS64, amd64_order, amd64_fix },
/*	{ EM_MIPS,	ELFCLASS32, mips_order, mips_fix }, */
/*	{ EM_MIPS64,	ELFCLASS64, mips64_order, mips64_fix }, */
	{ EM_PARISC,	ELFCLASS32, hppa_order, hppa_fix },
	{ EM_PARISC,	ELFCLASS64, hppa_order, hppa_fix },
/*	{ EM_PPC,	ELFCLASS32, ppc_order, ppc_fix }, */
/*	{ EM_PPC64,	ELFCLASS64, ppc64_order, ppc64_fix }, */
/*	{ EM_SPARC,	ELFCLASS32, sparc_order, sparc_fix }, */
	{ EM_SPARCV9,	ELFCLASS64, sparc64_order, sparc64_fix },
/*	{ EM_SH,	ELFCLASS32, sh_order, sh_fix }, */
	{ EM_ARM,	ELFCLASS32, arm_order, arm_fix },
/*	{ EM_68K,	ELFCLASS32, m68k_order, m68k_fix }, */
};
const int ldnarch = sizeof(ldarchs)/sizeof(ldarchs[0]);
const struct ldarch *ldarch;

int usage(void);
int libdir_add(const char *);
int obj_add(const char *, const char *, FILE *, off_t, struct objlist *);
int lib_add(const char *, FILE *fp);
int lib_namtab(const char *, FILE *, u_long, off_t, u_long);
int lib_symdef(const char *, FILE *, u_long);
int mmbr_name(struct ar_hdr *, char **, int, int *, FILE *);
struct headorder *ldorder(const struct ldarch *);
int order_check(struct objlist *, void *);
int uLD(const char *, const char *);

int
usage(void)
{
	extern char *__progname;
	fprintf(stderr, "usage: %s [file]\n", __progname);
	exit(1);
}

int
main(int argc, char *argv[])
{
	char output[MAXPATHLEN];
	u_int64_t *pst;
	FILE *fp;
	int ch, li;

	strlcpy(output, "a.out", sizeof output);
	libdir_add(_PATH_USRLIB);

	errors = 0;
	while ((ch = getopt_long(argc, argv, OPTSTRING, longopts, &li)) != -1)
		switch (ch) {
		case 0:
			/* check out 'li' to see what matched */
			break;

		case 'A':	/* set machine arch */
			break;

		case 'B':	/* static/shared/dynamic */
			if (!strcmp(argv[optind-1], "-Bstatic"))
				Bflag = 0;
			else if (!strcmp(argv[optind-1], "-Bshared"))
				Bflag = 2;
			else if (!strcmp(argv[optind-1], "-Bdynamic"))
				Bflag = 1;
			else
				errx(1, "invalid \"%s\" option",
				    argv[optind-1]);
			break;

		case 'c':	/* fini symbol */
			break;

		case 'C':	/* init symbol */
			break;

		case 'D':	/* define symbol */
			break;

		case 'd':	/* dynlink/FORCE_COMMON_ALLOCATION */
			if (!strcmp(argv[optind-1], "-dynamic-linker"))
				optind++;
			else {
				optind--;
				dflag++;
			}
			break;

		case 'e':	/* entry/export-dynamic */
			if (!strcmp(argv[optind-1], "-export-dynamic")) {
				optind--;
				export_dynamic++;
			} else
				entry_name = optarg;
			break;

		case 'E':	/* export dynamic symbols */
			export_dynamic++;
			break;

		case 'f':	/* add name to DT_AUXILIARY list */
			break;

		case 'F':	/* add name to DT_FILTER list */
			break;

		case 'h':	/* set the DT_SONAME field */
			break;

		case 'g':	/* compat: used to be to include debug info */
			break;

		case 'i':	/* perform incremental linking */
		case 'r':
			relocatable++;
			break;

		case 'L':	/* add to the library search path */
		case 'R':
		case 'Y':
			libdir_add(optarg);
			break;

		case 'l':
			/* end of options proceede with loading */
			break;

		case 'M':	/* print linking map to stdout/mapfile */
			if (!strcmp(argv[optind - 1], "-Map")) {
				mapfile = argv[optind];
				optind++;
			}
			printmap++;
			break;

		case 'n':	/* make NMAGIC output */
			magic = NMAGIC;
			break;

		case 'N':	/* make OMAGIC output */
			magic = OMAGIC;
			break;

		case 'o':	/* set output file name */
			strlcpy(output, optarg, sizeof output);
			break;

		case 'O':	/* optimise the otput binary (slow) */
			gc_sections++;
			break;

		case 'q':	/* leave the relocs in after full linking */
			break;

		case 'S':	/* strip only debugging info from the output */
			strip |= LD_DEBUG;
			break;

		case 's':	/* strip all symbol info from the output */
			strip |= LD_SYMTAB | LD_DEBUG;
			break;

		case 't':	/* trace files processed */
			trace++;
			break;

		case 'T':	/* linking script or section start */
			pst = NULL;
			if (!strcmp(argv[optind-1], "--section-start"))
				errx(1, "--section-start not supported");
			else if (!strcmp(argv[optind-1], "-Ttext"))
				pst = &start_text;
			else if (!strcmp(argv[optind-1], "-Tdata"))
				pst = &start_data;
			else if (!strcmp(argv[optind-1], "-Tbss"))
				pst = &start_bss;
			else
				errx(1, "invalid \"%s\" option",
				    argv[optind-1]);
			if (pst) {
				char *ep, *p;

				errno = 0;
				p = argv[optind];
				*pst = (u_int64_t)strtoull(p, &ep, 0);
				if (p[0] == '\0' || *ep != '\0')
					errx(1, "%s: invalid address", optarg);
				if (errno == ERANGE && *pst == ULLONG_MAX)
					errx(1, "%s: address is out", optarg);
				optind++;
			} else {
				errx(1, "scripts are not supported");
			}
			break;

		case 'u':	/* enter an undefined symbol */
			sym_undef(optarg);
			break;

		case 'v':
		case 'V':	/* print ld(1) version */
			break;

		case 'x':	/* discard all local symbols */
			Xflag = 2;
			break;

		case 'X':	/* discard only temp local symbols (L*) */
			Xflag = 1;
			break;

		case 'y':	/* trace all files for this symbol */
			break;

		case 'z':	/* special options */
			break;

		case 'Z':	/* make ZMAGIC output */
			magic = ZMAGIC;
			break;

		default:
			usage();
			break;
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		errx(1, "no input files");

	/* short cut for libraries building */
	if (relocatable && argc == 1)
		return uLD(*argv, output);

	/* make a "system" object for our self-defined sections & syms */
	sysobj.ol_path = output;
	sysobj.ol_nsect = 1;	/* .bss */
	TAILQ_INSERT_TAIL(&objlist, &sysobj, ol_entry);

	for (; argc--; argv++) {
		char armag[SARMAG];

		if (**argv == '-')
			switch (argv[0][1]) {
			case 'L':
				libdir_add(*argv + 2);
				continue;

			case 'l':	/* load the archive */
				lib_add(*argv, NULL);
				continue;

			case 'g':	/* compatibility */
				continue;

			case 'R':	/* only symbols from this file(s) */
				break;

			default:
				usage();
				break;
			}

		if (!(fp = fopen(*argv, "r")))
			err(1, "fopen: %s", *argv);

		if (fread(armag, sizeof armag, 1, fp) != 1) {
			if (feof(fp))
				errx(1, "%s: file is too short", *argv);
			err(1, "fread: %s", *argv);
		}

		fseek(fp, 0, SEEK_SET);
		if (!strncmp(armag, ARMAG, SARMAG))
			lib_add(*argv, fp);
		else {
			if (obj_add(*argv, NULL, fp, 0, NULL))
				return 1;
			fclose(fp);
		}
	}

	if (errors)
		return 1;

	/* and now do the jobs */
	if (elfclass == ELFCLASS32)
		return ldload32(output, ldmap32(ldorder(ldarch)));
	else
		return ldload64(output, ldmap64(ldorder(ldarch)));
}

/*
 * pre-scan thru the ldarch on the first encountered object
 * in order to generate necessary symbols and other gedoens
 */
const struct ldarch *
ldinit(void)
{
	const struct ldarch *lda;
	const struct ldorder *order;
	int i;

	for (lda = ldarchs, i = 0; i < ldnarch; lda++, i++)
		if (lda->la_mach == machine && lda->la_class == elfclass)
			break;

	if (i >= ldnarch) {
		warnx("unknown machine %d class %d", machine, elfclass);
		return NULL;
	}

	for (i = 0, order = lda->la_order;
	    order->ldo_order != ldo_kaput; order++, i++) {
		/* XXX we have to check ldo_flags as in ldmap() */

		switch (order->ldo_order) {
		/* these we do not care until later */
		case ldo_section:
		case ldo_interp:
		case ldo_ehfrh:
		case ldo_shstr:
		case ldo_symtab:
		case ldo_strtab:
			sysobj.ol_nsect++;
		case ldo_expr:
		case ldo_kaput:
			break;

		case ldo_option:
			/* TODO parse options */
			break;

		case ldo_symbol:
			if (order->ldo_flags & LD_ENTRY) {
				if (!entry_name)
					entry_name = order->ldo_name;
				sentry = sym_undef(entry_name);
				break;
			}
			break;
		}
	}

	if (!sysobj.ol_nsect)
		errx(1, "no sections defined");

	if (!(sysobj.ol_sections = calloc(sysobj.ol_nsect,
	    sizeof *sysobj.ol_sections)))
		err(1, "calloc");
	if (!(sysobj.ol_sects = calloc(sysobj.ol_nsect,
	    MAX(sizeof(Elf32_Shdr), sizeof(Elf64_Shdr)))))
		err(1, "calloc");
	/* os_sect is set in ldmap() */
	for (i = 0; i < sysobj.ol_nsect; i++) {
		sysobj.ol_sections[i].os_obj = &sysobj;
		TAILQ_INIT(&sysobj.ol_sections[i].os_syms);
	}

	return lda;
}

/*
 * add another library search dir (such as from -L)
 */
int
libdir_add(const char *path)
{
	char pathbuf[MAXPATHLEN];
	struct stat sb;
	struct pathlist *pl;
	size_t len;

	if (stat(path, &sb) < 0 || !S_ISDIR(sb.st_mode))
		return 1;

	if ((pl = calloc(1, sizeof *pl)) == NULL)
		err(1, "calloc");

	len = strlen(path);
	if (path[len - 1] != '/') {
		snprintf(pathbuf, sizeof pathbuf, "%s/", path);
		path = pathbuf;
		len++;
	}
	if ((pl->pl_path = strdup(path)) == NULL)
		err(1, "strdup");

	TAILQ_INSERT_TAIL(&libdirs, pl, pl_entry);
	return 0;
}

char *nametab;
long namtablen;

/*
 * search the library for objects resolving
 * currently undefined symbols;
 * if a library index (see ranlib(1)) is available
 * use that to pull needed objects and keep cycling
 * until no new symbols had been resolved;
 * otherwise scan whole library (once) checking
 * each member for symbols to resolve.
 */
int
lib_add(const char *path, FILE *fp)
{
	char armag[SARMAG];
	struct ar_hdr ah;
	off_t off, symoff;
	struct pathlist *pl;
	char *p;
	u_long len, symlen;
	int i, nlen, ltrace = trace;

	if (path[0] == '-' && path[1] == 'l')
		TAILQ_FOREACH(pl, &libdirs, pl_entry) {
			if (asprintf(&p, "%slib%s.a", pl->pl_path, path+2) < 0)
				err(1, "asprintf");
			if (ltrace)
				printf("%s (%s)\n", path, p);
			if (!access(p, F_OK)) {
				path = p;
				ltrace = 0;
				break;
			}
		}

	if (!fp && !(fp = fopen(path, "r")))
		err(1, "fopen: %s", path);

	if (ltrace)
		printf("%s\n", path);

	if (fread(armag, sizeof armag, 1, fp) != 1 ||
	    strncmp(armag, ARMAG, SARMAG))
		errx(1, "%s: not an archive", path);

	symoff = 0;
	symlen = 0;
	nametab = NULL;
	while (fread(&ah, sizeof ah, 1, fp) == 1) {
		if (memcmp(ah.ar_fmag, ARFMAG, sizeof ah.ar_fmag))
			errx(1, "%s: invalid archive header", path);

		off = ftello(fp);
		errno = 0;
		len = strtoul(ah.ar_size, NULL, 0);
		if (errno == ERANGE && len == ULONG_MAX)
			errx(1, "broken archive header");

		if (!strncmp(ah.ar_name, AR_NAMTAB, sizeof(AR_NAMTAB) - 1)) {
			/* un-ranliebt archive will do it the hard way */
			if (!symoff || !symlen)
				continue;

			if (!(nametab = malloc(len + 1)))
				err(1, "namtab malloc");

			if (fread(nametab, len, 1, fp) != 1)
				err(1, "%s: fread", path);

			nametab[len] = '\0';
			for (p = nametab, i = len; i--; p++)
				if (*p == '\n')
					*p = '\0';

			namtablen = len;
			if (lib_namtab(path, fp, len, symoff, symlen))
				exit(1);
			else
				break;
		}

		if (!strncmp(ah.ar_name, RANLIBMAG2, sizeof(RANLIBMAG2) - 1)) {
			symoff = off;
			symlen = len;
			if (fseeko(fp, off + len, SEEK_SET) < 0)
				err(1, "fseeko: %s", path);
			continue;
		}

		if (!strncmp(ah.ar_name, RANLIBMAG, sizeof(RANLIBMAG) - 1)) {
			if (lib_namtab(path, fp, len, symoff, symlen))
				exit(1);
			else
				break;
		}

		/* see if we have a symdef but no namtab */
		if (symoff) {
			if (lib_namtab(path, fp, len, symoff, symlen))
				exit(1);
			else
				break;
		}

		/*
		 * no symdef
		 * try to match symbols from every obj
		 * (vewy zlou)
		 */

		nlen = sizeof ah.ar_name;
		if (!(p = malloc(nlen)))
			err(1, "malloc");
		*p = '\0';
		if (mmbr_name(&ah, &p, 0, &nlen, fp))
			exit(1);

		errx(1, "%s: no symdef: not implemented", path);
	}

	if (nametab) {
		free(nametab);
		nametab = NULL;
	}
	fclose(fp);
	return 0;
}

/*
 * scan old (a.out/bsd) symdef index
 */
int
lib_symdef(const char *path, FILE *fp, u_long len)
{
	warnx("old symdef: not implemented");
	return -1;
}

/*
 * scan the new (sysv/elf) library index
 */
int
lib_namtab(const char *path, FILE *fp, u_long len, off_t symoff, u_long symlen)
{
	struct objlist *sol = TAILQ_LAST(&objlist, objhead);
	char *p, *pp;
	uint32_t num, *offs;
	int i, more;

	if (fseeko(fp, symoff, SEEK_SET) < 0)
		err(1, "fseeko: %s", path);

	if (!(pp = malloc(symlen)))
		err(1, "symdef malloc");

	if (fread(pp, symlen, 1, fp) != 1)
		err(1, "fread: %s", path);

	offs = (uint32_t *)pp;
	num = *offs++;
	num = betoh32(num);

	more = 1;
	while (more) {
		more = 0;
		for (i = 0, p = pp + (num + 1) * sizeof *offs;
		    i < num; i++, p += strlen(p) + 1) {
			struct ar_hdr mh;
			off_t foff;
			char *name;
			int nlen;

			if (!sym_isundef(p))
				continue;

			foff = betoh32(offs[i]);
			if (fseeko(fp, foff, SEEK_SET) < 0)
				err(1, "fseeko: %s", path);
			if (fread(&mh, sizeof mh, 1, fp) != 1)
				err(1, "fread: %s", path);
			if (memcmp(mh.ar_fmag, ARFMAG,
			    sizeof mh.ar_fmag))
				errx(1, "%s: invalid ar", path);
			nlen = sizeof mh.ar_name;
			if (!(name = malloc(nlen)))
				err(1, "malloc");
			*name = '\0';
			if (mmbr_name(&mh, &name, 0, &nlen, fp))
				return -1;

			/*
			 * this can be optimised by checking
			 * inside the obj_add if we added
			 * any new undef syms
			 */
			more = 1;
			obj_add(path, name, fp, foff + sizeof mh, sol);
			free(name);
		}
	}
	free(pp);

	return 0;
}

/*
 *	given the archive member header -- produce member name
 *	(pwnz0red shamelessly from nm.c)
 */
int
mmbr_name(struct ar_hdr *arh, char **name, int baselen, int *namelen, FILE *fp)
{
	char *p = *name + strlen(*name);
	long i;

	if (nametab && arh->ar_name[0] == '/') {
		int len;

		i = atol(&arh->ar_name[1]);
		if (i >= namtablen)
			errx(1, "corrupt ar member header");
		len = strlen(&nametab[i]);
		if (len > *namelen) {
			p -= (long)*name;
			if ((*name = realloc(*name, baselen+len)) == NULL)
				err(1, "realloc");
			*namelen = len;
			p += (long)*name;
		}
		strlcpy(p, &nametab[i], len);
		return 0;
	} else
#ifdef AR_EFMT1
	/*
	 * BSD 4.4 extended AR format: #1/<namelen>, with name as the
	 * first <namelen> bytes of the file
	 */
	if ((arh->ar_name[0] == '#') &&
	    (arh->ar_name[1] == '1') &&
	    (arh->ar_name[2] == '/') &&
	    (isdigit(arh->ar_name[3]))) {
		int len = atoi(&arh->ar_name[3]);

		if (len > *namelen) {
			p -= (long)*name;
			if ((*name = realloc(*name, baselen+len)) == NULL)
				err(1, "realloc");
			*namelen = len;
			p += (long)*name;
		}
		if (fread(p, len, 1, fp) != 1) {
			warnx("%s: premature EOF", *name);
			free(*name);
			return(1);
		}
		p += len;
	} else
#endif
	for (i = 0; i < sizeof(arh->ar_name); ++i)
		if (arh->ar_name[i] && arh->ar_name[i] != ' ')
			*p++ = arh->ar_name[i];
	*p = '\0';
	if (p[-1] == '/')
		*--p = '\0';

	return (0);
}

/*
 * scan through the global objects list
 * calling a function for each of which
 */
int
obj_foreach(int (*func)(struct objlist *, void *), void *v)
{
	struct objlist *ol;

	TAILQ_FOREACH(ol, &objlist, ol_entry) {
		if ((*func)(ol, v))
			return 1;
	}

	return 0;
}

/*
 * load an object from path (or path(name) for archive)
 * resolving undefined symbols an fetcing all info
 * needed for the second pass (mapping)
 */
int
obj_add(const char *path, const char *name, FILE *fp, off_t foff,
    struct objlist *sol)
{
	off_t ofoff;
	struct objlist *ol;

	if ((ol = calloc(1, sizeof *ol)) == NULL)
		err(1, "calloc");
	ol->ol_path = path;

	ofoff = 0;
	ol->ol_off = foff;
	if (foff) {
		ofoff = ftello(fp);
		if (fseeko(fp, foff, SEEK_SET) < 0)
			err(1, "fseeko: %s", path);
	}

	/* gotta be an ar member */
	if (name) {
		char b[MAXPATHLEN];
		snprintf(b, sizeof b, "%s(%s)", path, name);
		if (!(ol->ol_name = strdup(b)))
			err(1, "strdup");
	} else
		ol->ol_name = ol->ol_path;

	if (trace)
		printf("%s\n", ol->ol_name);

	if (fread(&ol->ol_hdr, sizeof ol->ol_hdr, 1, fp) != 1)
		err(1, "fread header: %s", ol->ol_name);

	if (!elf32_chk_header(&ol->ol_hdr.elf32)) {
		if (elf32_objadd(ol, fp, foff))
			exit(1);
	} else if (!elf64_chk_header(&ol->ol_hdr.elf64)) {
		if (elf64_objadd(ol, fp, foff))
			exit(1);
#if 0
	} else if (!BAD_OBJECT(ol->ol_hdr.aout)) {
		/* a.out goes here */
#endif
	} else
		errx(1, "%s: bad format", path);

	if (sol && randomise && randombit())
		TAILQ_INSERT_AFTER(&objlist, sol, ol, ol_entry);
	else
		TAILQ_INSERT_TAIL(&objlist, ol, ol_entry);
	fseeko(fp, ofoff, SEEK_SET);
	return 0;
}

/*
 * produce a loading order for the later mapping stage
 * this matches each template order for a given arch
 * against the object list
 * generate the resulting section name list
 */
struct headorder headorder;	/* XXX until struct arg/ret works */

struct headorder *
ldorder(const struct ldarch *lda)
{
	const struct ldorder *order;
	struct ldorder *neworder, *ssorder;
	struct symlist *sym;
	size_t n;
	int i;

	ssorder = 0;
	TAILQ_INIT(&headorder);
	for (i = 0, order = lda->la_order;
	    order->ldo_order != ldo_kaput; order++, i++) {

		if (!(Bflag || pie) && (order->ldo_flags & LD_DYNAMIC))
			continue;

		if (strip & order->ldo_flags)
			continue;
		if (strip == 2 && (order->ldo_order == ldo_symtab ||
		    order->ldo_order == ldo_strtab))
			continue;

		if (magic == OMAGIC && (order->ldo_flags & LD_NOOMAGIC))
			continue;
		if (magic == NMAGIC && (order->ldo_flags & LD_NONMAGIC))
			continue;
		if (magic == ZMAGIC && (order->ldo_flags & LD_NOZMAGIC))
			continue;

		switch (order->ldo_order) {
		/* these are handled in ldinit */
		case ldo_option:
			break;

		case ldo_expr:
			neworder = order_clone(lda, order);
			TAILQ_INSERT_TAIL(&headorder, neworder, ldo_entry);
			break;

		case ldo_section:
			neworder = order_clone(lda, order);
			if (obj_foreach(elfclass == ELFCLASS32?
			    ld32order_obj : ld64order_obj, neworder)) {
				free(neworder);
				break;
			}
			TAILQ_INSERT_TAIL(&headorder, neworder, ldo_entry);
			break;

		case ldo_symbol:
			/* entry has been already handled in ldinit() */
			if (order->ldo_flags & LD_ENTRY)
				break;
			if ((sym = sym_isdefined(order->ldo_name, NULL))) {
				warnx("%s: already defined in %s",
				    order->ldo_name,
				    sym->sl_sect->os_obj->ol_name);
				errors++;
				break;
			}
			neworder = order_clone(lda, order);
			neworder->ldo_wurst = elfclass == ELFCLASS32?
			    elf32_absadd(neworder->ldo_name, STB_GLOBAL) :
			    elf64_absadd(neworder->ldo_name, STB_GLOBAL);
			TAILQ_INSERT_TAIL(&headorder, neworder, ldo_entry);
			break;

		case ldo_interp:
			neworder = order_clone(lda, order);
			neworder->ldo_wurst = strdup(LD_INTERP);
			neworder->ldo_wsize = ALIGN(neworder->ldo_wsize);
			TAILQ_INSERT_TAIL(&headorder, neworder, ldo_entry);
			break;

		case ldo_ehfrh:
			neworder = order_clone(lda, order);
			neworder->ldo_wurst = calloc(1, 4);
			neworder->ldo_wsize = 4;
			{
				uint8_t *p = neworder->ldo_wurst;
				p[0] = 1;	/* version */
#define	DW_EH_PE_omit	0xff
				p[1] = DW_EH_PE_omit;
				p[2] = DW_EH_PE_omit;
				p[3] = DW_EH_PE_omit;
			}
			TAILQ_INSERT_TAIL(&headorder, neworder, ldo_entry);
			break;

		case ldo_symtab:
		case ldo_strtab:
		case ldo_shstr:
			neworder = order_clone(lda, order);
			if (order->ldo_order == ldo_shstr)
				ssorder = neworder;
			TAILQ_INSERT_TAIL(&headorder, neworder, ldo_entry);
			break;

		case ldo_kaput:
			break;
		}
	}

	/* check for disordered sections */
	obj_foreach(order_check, NULL);

	n = 1;
	TAILQ_FOREACH(order, &headorder, ldo_entry)
		n += strlen(order->ldo_name) + 1;
	ssorder->ldo_wsize = SHALIGN(n);
	if (!(ssorder->ldo_wurst = malloc(ssorder->ldo_wsize)))
		err(1, "malloc");
	sysobj.ol_snames = ssorder->ldo_wurst;

	return &headorder;
}

int
order_check(struct objlist *ol, void *v)
{
	struct section *os = ol->ol_sections;
	int i, n;

        n = ol->ol_nsect;
	for (i = 1, os++; i < n; i++, os++) {
		if (os->os_flags & SECTION_ORDER)
			continue;

		if (os->os_flags & SHF_ALLOC)
			warnx("%s: section \"%s\" not loaded",
			    ol->ol_name, os->os_name);
	}

	return 0;
}

/*
 * remove unreferenced sections from the order;
 * only called if --gc-sections was specified
 */
struct headorder *
elf_gcs(struct headorder *headorder)
{
	struct ldorder *ord;
	struct section *os, *next;
	int changed;

	if (!sentry || !sentry->sl_sect)
		errx(1, "entry point not defined");
	sentry->sl_sect->os_flags |= SECTION_USED;

	/* pass 1: roll thru the order marking sections as used */
	changed = 1;
	TAILQ_FOREACH(ord, headorder, ldo_entry) {
		if (changed) {
			ord = TAILQ_FIRST(headorder);
			changed = 0;
		}

		if (ord->ldo_order != ldo_section)
			continue;

		TAILQ_FOREACH(os, &ord->ldo_seclst, os_entry) {
			struct relist *rp, *er;
			if (!(os->os_flags & SECTION_USED))
				continue;

			for (rp = os->os_rels, er = &os->os_rels[os->os_nrls];
			    rp < er; rp++)
				if (rp->rl_sym->sl_sect &&
				    !(rp->rl_sym->sl_sect->os_flags &
				    SECTION_USED)) {
					changed = 1;
					rp->rl_sym->sl_sect->os_flags |=
					    SECTION_USED;
				}
		}
	}

	/* pass 2: roll thru the order removing unused sections */
	TAILQ_FOREACH(ord, headorder, ldo_entry) {
		if (ord->ldo_order != ldo_section)
			continue;

		for(os = TAILQ_FIRST(&ord->ldo_seclst);
		    os != TAILQ_END(&ord->ldo_seclst); os = next) {
			next = TAILQ_NEXT(os, os_entry);

			/*
			 * simply drop the section from the list
			 * and avoid the free(3) hustle as we are
			 * unlikely to need more memory and exit(1)
			 * brings freedom to everyone
			 */
			if (!(os->os_flags & SECTION_USED))
				TAILQ_REMOVE(&ord->ldo_seclst, os, os_entry);
		}
	}

	return headorder;
}

/*
 * a wrapper for the micro-linker (ld2.c)
 * read the whole object into memory and
 * call appropriate worker (32/64)
 * upon success ump out the result
 */
int
uLD(const char *name, const char *output)
{
	union banners {
		struct exec aout;
		Elf32_Ehdr elf32;
		Elf64_Ehdr elf64;
	} *h;
	struct stat sb;
	FILE *fp, *ofp;
	char *v;
	int rv, size;

	if (!(fp = fopen(name, "r")))
		err(1, "fopen: %s", name);

	if (fstat(fileno(fp), &sb) < 0)
		err(1, "fstat: %s", name);

	if (sb.st_size > INT_MAX)
		errx(1, "%s: file is too big", name);

	if (!(v = malloc(sb.st_size)))
		err(1, "malloc %d bytes", (int)sb.st_size);

	if (fread(v, sb.st_size, 1, fp) != 1) {
		if (feof(fp))
			errx(1, "%s: shrinkage", name);
		else
			err(1, "fread: %s", name);
	}

	if (!(ofp = fopen(output, "w")))
		err(1, "fopen: %s", output);

	size = sb.st_size;
	h = (union banners *)v;
	if (IS_ELF(h->elf32) &&
	    h->elf32.e_ident[EI_CLASS] == ELFCLASS32 &&
	    h->elf32.e_ident[EI_VERSION] == ELF_TARG_VER)
		rv = uLD32(name, v, &size, Xflag);
	else if (IS_ELF(h->elf64) &&
	    h->elf64.e_ident[EI_CLASS] == ELFCLASS64 &&
	    h->elf64.e_ident[EI_VERSION] == ELF_TARG_VER)
		rv = uLD64(name, v, &size, Xflag);
	else
		return 1;

	if (!rv && fwrite(v, size, 1, ofp) != 1)
		err(1, "fwrite: %s", name);

	return 0;
}
