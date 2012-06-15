/*
 * Copyright (c) 2003-2009 Michael Shalayeff
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Hans Huebner.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char copyright[] =
"Copyright (c) 2003-2009 Michael Shalayeff. All rights reserved.\n"
"@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static const char sccsid[] = "@(#)nm.c	8.1 (Berkeley) 6/6/93";
#else
static const char rcsid[] = "$ABSD: nm.c,v 1.17 2012/06/15 16:35:15 mickey Exp $";
#endif
#endif

#include <sys/param.h>
#include <sys/mman.h>
#include <a.out.h>
#include <elf_abi.h>
#include <stab.h>
#include <ar.h>
#include <ranlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <ctype.h>
#include <link.h>
#ifdef __ELF__
#include <link_aout.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <elfuncs.h>
#include "util.h"

/* XXX get shared code to handle a.out byte-order swaps */
#include "byte.c"

union hdr {
	struct exec aout;
	Elf32_Ehdr elf32;
	Elf64_Ehdr elf64;
};

/* a funky nlist overload for reading 32bit a.out on 64bit toys */
struct nlist32 {
	u_int32_t	strx;
	u_int8_t	type;
	u_int8_t	other;
	u_int16_t	desc;
	u_int32_t	value;
} __packed;

int armap;
int demangle;
int non_object_warning;
int print_only_external_symbols;
int print_only_undefined_symbols;
int print_all_symbols;
int print_file_each_line;
int show_extensions;
int issize;
int usemmap = 1;

/* size vars */
unsigned long total_text, total_data, total_bss, total_total;
int non_object_warning, print_totals;

int rev;
int fname(const void *, const void *);
int rname(const void *, const void *);
int value(const void *, const void *);
int (*sfunc)(const void *, const void *) = fname;
char *otherstring(struct nlist *);
char *typestring(unsigned int);
char typeletter(struct nlist *, int);

/* some macros for symbol type (nlist.n_type) handling */
#define	IS_DEBUGGER_SYMBOL(x)	((x) & N_STAB)
#define	IS_EXTERNAL(x)		((x) & N_EXT)
#define	SYMBOL_TYPE(x)		((x) & (N_TYPE | N_STAB))

void	 pipe2cppfilt(void);
void	 usage(void);
char	*symname(struct nlist *, int);
int	process_file(int, const char *);
int	show_archive(int, const char *, FILE *);
int	show_file(int, int, const char *, FILE *fp, off_t, union hdr *);
void	print_symbol(const char *, struct nlist *, int);

#define	OPTSTRING_NM	"aABCegnoprsuvVw"
const struct option longopts_nm[] = {
	{ "debug-syms",		no_argument,		0,	'a' },
	{ "demangle",		no_argument,		0,	'C' },
/*	{ "dynamic",		no_argument,		0,	'D' }, */
	{ "extern-only",	no_argument,		0,	'g' },
/*	{ "line-numbers",	no_argument,		0,	'l' }, */
	{ "no-sort",		no_argument,		0,	'p' },
	{ "numeric-sort",	no_argument,		0,	'n' },
	{ "print-armap",	no_argument,		0,	's' },
	{ "print-file-name",	no_argument,		0,	'o' },
	{ "reverse-sort",	no_argument,		0,	'r' },
/*	{ "size-sort",		no_argument,		&szval,	1 }, */
	{ "undefined-only",	no_argument,		0,	'u' },
	{ "version",		no_argument,		0,	'V' },
	{ "help",		no_argument,		0,	'?' },
	{ NULL }
};

/*
 * main()
 *	parse command line, execute process_file() for each file
 *	specified on the command line.
 */
int
main(int argc, char *argv[])
{
	extern char *__progname;
	extern int optind;
	const char *optstr;
	const struct option *lopts;
	int ch, eval;

	optstr = OPTSTRING_NM;
	lopts = longopts_nm;
	if (!strcmp(__progname, "size")) {
		issize++;
		optstr = "tw";
		lopts = NULL;
	}

	while ((ch = getopt_long(argc, argv, optstr, lopts, NULL)) != -1) {
		switch (ch) {
		case 'a':
			print_all_symbols = 1;
			break;
		case 'B':
			/* no-op, compat with gnu-nm */
			break;
		case 'C':
			demangle = 1;
			break;
		case 'e':
			show_extensions = 1;
			break;
		case 'g':
			print_only_external_symbols = 1;
			break;
		case 'n':
		case 'v':
			sfunc = value;
			break;
		case 'A':
		case 'o':
			print_file_each_line = 1;
			break;
		case 'p':
			sfunc = NULL;
			break;
		case 'r':
			rev = 1;
			break;
		case 's':
			armap = 1;
			break;
		case 'u':
			print_only_undefined_symbols = 1;
			break;
		case 'V':
			fprintf(stderr, "%s\n", rcsid);
			exit(0);
		case 'w':
			non_object_warning = 1;
			break;
		case 't':
			if (issize) {
				print_totals = 1;
				break;
			}
		case '?':
		default:
			usage();
		}
	}

	if (demangle)
		pipe2cppfilt();
	argv += optind;
	argc -= optind;

	if (rev && sfunc == fname)
		sfunc = rname;

	eval = 0;
	if (*argv)
		do {
			eval |= process_file(argc, *argv);
		} while (*++argv);
	else
		eval |= process_file(1, "a.out");

	if (issize && print_totals)
		printf("\n%lu\t%lu\t%lu\t%lu\t%lx\tTOTAL\n",
		    total_text, total_data, total_bss,
		    total_total, total_total);
	exit(eval);
}

/*
 * process_file()
 *	show symbols in the file given as an argument.  Accepts archive and
 *	object files as input.
 */
int
process_file(int count, const char *fname)
{
	union hdr exec_head;
	FILE *fp;
	int retval;
	size_t bytes;
	char magic[SARMAG];

	if (!(fp = fopen(fname, "r"))) {
		warn("cannot read %s", fname);
		return(1);
	}

	/*
	 * first check whether this is an object file - read a object
	 * header, and skip back to the beginning
	 */
	bzero(&exec_head, sizeof(exec_head));
	bytes = fread((char *)&exec_head, 1, sizeof(exec_head), fp);
	if (bytes < sizeof(exec_head)) {
		if (bytes < sizeof(exec_head.aout) || IS_ELF(exec_head.elf32)) {
			warnx("%s: bad format", fname);
			(void)fclose(fp);
			return(1);
		}
	}
	rewind(fp);

	/* this could be an archive */
	if (!IS_ELF(exec_head.elf32) && N_BADMAG(exec_head.aout)) {
		if (fread(magic, sizeof magic, 1, fp) != 1 ||
		    strncmp(magic, ARMAG, SARMAG)) {
			warnx("%s: not object file or archive", fname);
			(void)fclose(fp);
			return(1);
		}

		if (!issize && count > 1)
			(void)printf("\n%s:\n", fname);

		retval = show_archive(count, fname, fp);
	} else
		retval = show_file(count, 1, fname, fp, 0, &exec_head);
	(void)fclose(fp);
	return(retval);
}

char *nametab;
long namtablen;

/*
 *
 *	given the archive member header -- produce member name
 */
int
mmbr_name(struct ar_hdr *arh, char **name, int baselen, int *namelen, FILE *fp)
{
	char *p = *name + strlen(*name);
	long i;

	if (nametab && arh->ar_name[0] == '/') {
		int len;

		i = atol(&arh->ar_name[1]);
		if (i >= namtablen) {
			warnx("corrupt ar member header");
			return 1;
		}
		len = strlen(&nametab[i]);
		if (len > *namelen) {
			p -= (long)*name;
			if ((*name = realloc(*name, baselen+len)) == NULL)
				err(1, "realloc");
			*namelen = len;
			p += (long)*name;
		}
		strlcpy(p, &nametab[i], len);
		p += len;
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
 * show_symtab()
 *	show archive ranlib index (fs5)
 */
int
show_symtab(off_t off, u_long len, const char *name, FILE *fp)
{
	struct ar_hdr ar_head;
	int *symtab, *ps;
	char *strtab, *p;
	int num, rval = 0;
	int namelen;

	MMAP(symtab, len, PROT_READ, MAP_PRIVATE|MAP_FILE, fileno(fp), off);
	if (symtab == MAP_FAILED)
		return (1);

	namelen = sizeof(ar_head.ar_name);
	if ((p = malloc(sizeof(ar_head.ar_name))) == NULL) {
		warn("%s: malloc", name);
		MUNMAP(symtab, len);
	}

	printf("\nArchive index:\n");
	num = betoh32(*symtab);
	strtab = (char *)(symtab + num + 1);
	for (ps = symtab + 1; num--; ps++, strtab += strlen(strtab) + 1) {
		if (pread(fileno(fp), &ar_head, sizeof ar_head,
		    betoh32(*ps)) != sizeof ar_head ||
		    memcmp(ar_head.ar_fmag, ARFMAG, sizeof(ar_head.ar_fmag))) {
			warnx("%s: member pread", name);
			rval = 1;
			break;
		}

		*p = '\0';
		if (mmbr_name(&ar_head, &p, 0, &namelen, fp)) {
			rval = 1;
			break;
		}

		printf("%s in %s\n", strtab, p);
	}

	free(p);
	MUNMAP(symtab, len);
	return (rval);
}

/*
 * show_symdef()
 *	show archive ranlib index (gob)
 */
int
show_symdef(off_t off, u_long len, const char *name, FILE *fp)
{
	struct ranlib *prn, *eprn;
	struct ar_hdr ar_head;
	void *symdef;
	char *strtab, *p;
	u_long size;
	int namelen, rval = 0;

	MMAP(symdef, len, PROT_READ, MAP_PRIVATE|MAP_FILE, fileno(fp), off);
	if (symdef == MAP_FAILED)
		return (1);
	if (usemmap)
		(void)madvise(symdef, len, MADV_SEQUENTIAL);

	namelen = sizeof(ar_head.ar_name);
	if ((p = malloc(sizeof(ar_head.ar_name))) == NULL) {
		warn("%s: malloc", name);
		MUNMAP(symdef, len);
		return (1);
	}

	size = *(u_long *)symdef;
	prn = symdef + sizeof(u_long);
	eprn = prn + size / sizeof(*prn);
	strtab = symdef + sizeof(u_long) + size + sizeof(u_long);

	printf("\nArchive index:\n");
	for (; prn < eprn; prn++) {
		if (fseeko(fp, prn->ran_off, SEEK_SET)) {
			warn("%s: fseeko", name);
			rval = 1;
			break;
		}

		if (fread(&ar_head, sizeof(ar_head), 1, fp) != 1 ||
		    memcmp(ar_head.ar_fmag, ARFMAG, sizeof(ar_head.ar_fmag))) {
			warnx("%s: member fseeko", name);
			rval = 1;
			break;
		}

		*p = '\0';
		if (mmbr_name(&ar_head, &p, 0, &namelen, fp)) {
			rval = 1;
			break;
		}

		printf("%s in %s\n", strtab + prn->ran_un.ran_strx, p);
	}

	free(p);
	MUNMAP(symdef, len);
	return (rval);
}

/*
 * show_archive()
 *	show symbols in the given archive file
 */
int
show_archive(int count, const char *fname, FILE *fp)
{
	struct ar_hdr ar_head;
	union hdr exec_head;
	off_t last_ar_off, foff, symtaboff;
	char *name;
	u_long mmbrlen, symtablen;
	int i, rval, baselen, namelen;

	baselen = strlen(fname) + 3;
	namelen = sizeof(ar_head.ar_name);
	if ((name = malloc(baselen + namelen)) == NULL)
		err(1, NULL);

	rval = 0;
	nametab = NULL;
	symtaboff = 0;
	symtablen = 0;

	/* while there are more entries in the archive */
	while (fread(&ar_head, sizeof(ar_head), 1, fp) == 1) {
		/* bad archive entry - stop processing this archive */
		if (memcmp(ar_head.ar_fmag, ARFMAG, sizeof(ar_head.ar_fmag))) {
			warnx("%s: bad format archive header", fname);
			rval = 1;
			break;
		}

		/* remember start position of current archive object */
		last_ar_off = ftello(fp);
		mmbrlen = atol(ar_head.ar_size);

		if (strncmp(ar_head.ar_name, RANLIBMAG,
		    sizeof(RANLIBMAG) - 1) == 0) {
			if (!issize && armap &&
			    show_symdef(last_ar_off, mmbrlen, fname, fp)) {
				rval = 1;
				break;
			}
			goto skip;
			/* load the Sys5 long names table */
		} else if (strncmp(ar_head.ar_name, AR_NAMTAB,
		    sizeof(AR_NAMTAB) - 1) == 0) {
			char *p;

			if ((nametab = malloc(mmbrlen + 1)) == NULL) {
				warn("%s: nametab", fname);
 				rval = 1;
 				break;
 			}
			nametab[mmbrlen] = '\0';

			if (fread(nametab, mmbrlen, 1, fp) != 1) {
				warnx("%s: premature EOF", fname);
 				rval = 1;
 				break;
 			}
 
			nametab[mmbrlen] = '\0';
			for (p = nametab, i = mmbrlen; i--; p++)
				if (*p == '\n')
					*p = '\0';
			namtablen = mmbrlen;
 			if (issize || !armap || !symtablen || !symtaboff)
 				goto skip;
		} else if (strncmp(ar_head.ar_name, RANLIBMAG2,
		    sizeof(RANLIBMAG2) - 1) == 0) {
			/* if nametab hasn't been seen yet -- doit later */
			if (!nametab) {
				symtablen = mmbrlen;
				symtaboff = last_ar_off;
				goto skip;
			}
		}

		if (!issize && armap && symtablen && symtaboff) {
			if (show_symtab(symtaboff, symtablen, fname, fp)) {
				rval = 1;
				break;
			} else {
				symtaboff = 0;
				symtablen = 0;
			}
		}

		/*
		 * construct a name of the form "archive.a:obj.o:" for the
		 * current archive entry if the object name is to be printed
		 * on each output line
		 */
		*name = '\0';
		if (count > 1)
			snprintf(name, baselen - 1, "%s:", fname);

		if (mmbr_name(&ar_head, &name, baselen, &namelen, fp)) {
			rval = 1;
			break;
		}

		foff = ftello(fp);

		/* get and check current object's header */
		if (fread((char *)&exec_head, sizeof(exec_head),
		    (size_t)1, fp) != 1) {
			warnx("%s: premature EOF", fname);
			rval = 1;
			break;
		}

		rval |= show_file(2, non_object_warning, name, fp, foff, &exec_head);
		/*
		 * skip to next archive object
		 */
#define	even(x)	(((x) + 1) & ~1)
skip:		if (fseeko(fp, last_ar_off + even(mmbrlen), SEEK_SET) < 0) {
			warn("%s", fname);
			rval = 1;
			break;
		}
	}
	if (nametab) {
		free(nametab);
		nametab = NULL;
	}
	free(name);
	return(rval);
}

int nrawnames;

int
elf_symadd(struct elf_symtab *es, int si, void *sym, void *v)
{
	struct nlist *nl;
	struct nlist **np = v;

	if (!*np && !(*np = calloc(es->nsyms, sizeof **np)))
		err(1, "calloc");

	nl = &(*np)[nrawnames++];
	if (((Elf_Ehdr *)es->ehdr)->e_ident[EI_CLASS] == ELFCLASS32)
		elf32_2nlist(sym, es->ehdr, es->shdr, es->shstr, nl);
	else
		elf64_2nlist(sym, es->ehdr, es->shdr, es->shstr, nl);
	nl->n_un.n_name = es->stab + nl->n_un.n_strx;
	if (*nl->n_un.n_name == '\0')
		nrawnames--;
	return 0;
}

/*
 * show_file()
 *	show symbols from the object file pointed to by fp.  The current
 *	file pointer for fp is expected to be at the beginning of an object
 *	file header.
 */
int
show_file(int count, int warn_fmt, const char *name, FILE *fp, off_t foff, union hdr *head)
{
	u_long text, data, bss, total;
	struct nlist *np, *names, **snames;
	char *stab;
	int i, aout, nnames;
	size_t stabsize;
	off_t staboff;

	aout = 0;
	if (!elf32_chk_header(&head->elf32)) {
		struct elf_symtab es;

		elf32_fix_header(&head->elf32);

		es.name = name;
		es.ehdr = &head->elf32;
		if (head->elf32.e_ehsize < sizeof head->elf32) {
			warnx("%s: ELF header is too short", name);
			return 1;
		}

		if (!(es.shdr = elf32_load_shdrs(name, fp, foff, es.ehdr)))
			return (1);
		elf32_fix_shdrs(es.ehdr, es.shdr);
		es.shstr = NULL;

		if (issize)
			i = elf32_size(es.ehdr, es.shdr, &text, &data, &bss);
		else {
			nrawnames = 0;
			names = NULL;
			i = elf32_symload(&es, fp, foff, elf_symadd, &names);
			stab = es.stab;
			stabsize = es.stabsz;
		}
		free(es.shstr);
		free(es.shdr);
		if (i)
			return (i);

	} else if (!elf64_chk_header(&head->elf64)) {
		struct elf_symtab es;

		elf64_fix_header(&head->elf64);

		es.name = name;
		es.ehdr = &head->elf64;
		if (head->elf64.e_ehsize < sizeof head->elf64) {
			warnx("%s: ELF header is too short", name);
			return 1;
		}
		if (!(es.shdr = elf64_load_shdrs(name, fp, foff, es.ehdr)))
			return (1);
		elf64_fix_shdrs(es.ehdr, es.shdr);
		es.shstr = NULL;

		if (issize)
			i = elf64_size(es.ehdr, es.shdr, &text, &data, &bss);
		else {
			nrawnames = 0;
			names = NULL;
			i = elf64_symload(&es, fp, foff, elf_symadd, &names);
			stab = es.stab;
			stabsize = es.stabsz;
		}
		free(es.shstr);
		free(es.shdr);
		if (i)
			return (i);

	} else if (BAD_OBJECT(head->aout)) {
		if (warn_fmt)
			warnx("%s: bad format", name);
		return (1);
	} else do {
		u_int32_t w;

		aout++;

		fix_header_order(&head->aout);

		if (issize) {
			text = head->aout.a_text;
			data = head->aout.a_data;
			bss = head->aout.a_bss;
			break;
		}

		/* stop if the object file contains no symbol table */
		if (!head->aout.a_syms) {
			warnx("%s: no name list", name);
			return(1);
		}

		if (fseeko(fp, foff + N_SYMOFF(head->aout), SEEK_SET)) {
			warn("%s", name);
			return(1);
		}

#ifdef __LP64__
		nrawnames = head->aout.a_syms / sizeof(struct nlist32);
#else
		nrawnames = head->aout.a_syms / sizeof(*names);
#endif
		/* get memory for the symbol table */
		if ((names = calloc(nrawnames, sizeof(struct nlist))) == NULL) {
			warn("%s: malloc names", name);
			return (1);
		}

#ifdef __LP64__
		for (np = names, i = nrawnames; i--; np++) {
			struct nlist32 nl32;

			if (fread(&nl32, sizeof(nl32), 1, fp) != 1) {
				warnx("%s: cannot read symbol table", name);
				free(names);
				return (1);
			}
			np->n_type = nl32.type;
			np->n_other = nl32.other;
			if (byte_sex(N_GETMID(head->aout)) != BYTE_ORDER) {
				np->n_un.n_strx = swap32(nl32.strx);
				np->n_desc = swap16(nl32.desc);
				np->n_value = swap32(nl32.value);
			} else {
				np->n_un.n_strx = nl32.strx;
				np->n_desc = nl32.desc;
				np->n_value = nl32.value;
			}
		}
#else
		if (fread(names, head->aout.a_syms, 1, fp) != 1) {
			warnx("%s: cannot read symbol table", name);
			free(names);
			return (1);
		}
		fix_nlists_order(names, nrawnames, N_GETMID(head->aout));
#endif

		staboff = ftello(fp);
		/*
		 * Following the symbol table comes the string table.
		 * The first 4-byte-integer gives the total size of the
		 * string table _including_ the size specification itself.
		 */
		if (fread(&w, sizeof(w), (size_t)1, fp) != 1) {
			warnx("%s: cannot read stab size", name);
			free(names);
			return(1);
		}
		stabsize = fix_32_order(w, N_GETMID(head->aout));
		MMAP(stab, stabsize, PROT_READ, MAP_PRIVATE|MAP_FILE,
		    fileno(fp), staboff);
		if (stab == MAP_FAILED) {
			free(names);
			return (1);
		}

		stabsize -= 4;		/* we already have the size */
		for (np = names, i = nnames = 0; i < nrawnames; np++, i++) {
			/*
			 * make n_un.n_name a character pointer by adding
			 * the string table's base to n_un.n_strx
			 *
			 * don't mess with zero offsets
			 */
			if (np->n_un.n_strx)
				np->n_un.n_name = stab + np->n_un.n_strx;
			else
				np->n_un.n_name = "";
		}
	} while (0);

	if (issize) {
		static int first = 1;

		if (first) {
			first = 0;
			printf("text\tdata\tbss\tdec\thex\n");
		}

		total = text + data + bss;
		printf("%lu\t%lu\t%lu\t%lu\t%lx",
		    text, data, bss, total, total);
		if (count > 1)
			(void)printf("\t%s", name);

		total_text += text;
		total_data += data;
		total_bss += bss;
		total_total += total;

		printf("\n");
		return (0);
	}
	/* else we are nm */

	/*
	 * it seems that string table is sequential
	 * relative to the symbol table order
	 */
	if (sfunc == NULL && usemmap)
		(void)madvise(stab, stabsize, MADV_SEQUENTIAL);

	if ((snames = calloc(nrawnames, sizeof *snames)) == NULL) {
		warn("%s: malloc snames", name);
		free(names);
		return (1);
	}

	/*
	 * fix up the symbol table and filter out unwanted entries
	 *
	 * common symbols are characterized by a n_type of N_UNDF and a
	 * non-zero n_value -- change n_type to N_COMM for all such
	 * symbols to make life easier later.
	 *
	 * filter out all entries which we don't want to print anyway
	 */
	for (np = names, i = nnames = 0; i < nrawnames; np++, i++) {
		if (aout && SYMBOL_TYPE(np->n_type) == N_UNDF && np->n_value)
			np->n_type = N_COMM | (np->n_type & N_EXT);
		if (!print_all_symbols && IS_DEBUGGER_SYMBOL(np->n_type))
			continue;
		if (print_only_external_symbols && !IS_EXTERNAL(np->n_type))
			continue;
		if (print_only_undefined_symbols &&
		    SYMBOL_TYPE(np->n_type) != N_UNDF)
			continue;

		snames[nnames++] = np;
	}

	/* sort the symbol table if applicable */
	if (sfunc)
		qsort(snames, (size_t)nnames, sizeof(*snames), sfunc);

	if (count > 1)
		(void)printf("\n%s:\n", name);

	/* print out symbols */
	for (i = 0; i < nnames; i++) {
		if (show_extensions && snames[i] != names &&
		    SYMBOL_TYPE((snames[i] -1)->n_type) == N_INDR)
			continue;
		print_symbol(name, snames[i], aout);
	}

	free(snames);
	free(names);
	if (aout)
		MUNMAP(stab, stabsize);
	else
		free(stab);
	return(0);
}

char *
symname(struct nlist *sym, int aout)
{
	if (demangle && sym->n_un.n_name[0] == '_' && aout)
		return sym->n_un.n_name + 1;
	else
		return sym->n_un.n_name;
}

/*
 * print_symbol()
 *	show one symbol
 */
void
print_symbol(const char *name, struct nlist *sym, int aout)
{
	if (print_file_each_line)
		(void)printf("%s:", name);

	/*
	 * handle undefined-only format especially (no space is
	 * left for symbol values, no type field is printed)
	 */
	if (!print_only_undefined_symbols) {
		/* print symbol's value */
		if (SYMBOL_TYPE(sym->n_type) == N_UNDF ||
		    (show_extensions && SYMBOL_TYPE(sym->n_type) == N_INDR &&
		    sym->n_value == 0))
			(void)printf("        ");
		else
			(void)printf("%08lx", sym->n_value);

		/* print type information */
		if (IS_DEBUGGER_SYMBOL(sym->n_type))
			(void)printf(" - %02x %04x %5s ", sym->n_other,
			    sym->n_desc&0xffff, typestring(sym->n_type));
		else if (show_extensions)
			(void)printf(" %c%2s ", typeletter(sym, aout),
			    otherstring(sym));
		else
			(void)printf(" %c ", typeletter(sym, aout));
	}

	if (SYMBOL_TYPE(sym->n_type) == N_INDR && show_extensions) {
		printf("%s -> %s\n", symname(sym, aout), symname(sym+1, aout));
	}
	else
		(void)puts(symname(sym, aout));
}

char *
otherstring(struct nlist *sym)
{
	static char buf[3];
	char *result;

	result = buf;

	if (N_BIND(sym) == BIND_WEAK)
		*result++ = 'w';
	if (N_AUX(sym) == AUX_OBJECT)
		*result++ = 'o';
	else if (N_AUX(sym) == AUX_FUNC)
		*result++ = 'f';
	*result++ = 0;
	return buf;
}

/*
 * typestring()
 *	return the a description string for an STAB entry
 */
char *
typestring(unsigned int type)
{
	switch(type) {
	case N_BCOMM:
		return("BCOMM");
	case N_ECOML:
		return("ECOML");
	case N_ECOMM:
		return("ECOMM");
	case N_ENTRY:
		return("ENTRY");
	case N_FNAME:
		return("FNAME");
	case N_FUN:
		return("FUN");
	case N_GSYM:
		return("GSYM");
	case N_LBRAC:
		return("LBRAC");
	case N_LCSYM:
		return("LCSYM");
	case N_LENG:
		return("LENG");
	case N_LSYM:
		return("LSYM");
	case N_PC:
		return("PC");
	case N_PSYM:
		return("PSYM");
	case N_RBRAC:
		return("RBRAC");
	case N_RSYM:
		return("RSYM");
	case N_SLINE:
		return("SLINE");
	case N_SO:
		return("SO");
	case N_SOL:
		return("SOL");
	case N_SSYM:
		return("SSYM");
	case N_STSYM:
		return("STSYM");
	}
	return("???");
}

/*
 * typeletter()
 *	return a description letter for the given basic type code of an
 *	symbol table entry.  The return value will be upper case for
 *	external, lower case for internal symbols.
 */
char
typeletter(struct nlist *np, int aout)
{
	int ext = IS_EXTERNAL(np->n_type);

	if (!aout && !IS_DEBUGGER_SYMBOL(np->n_type) && np->n_other)
		return np->n_other;

	switch(SYMBOL_TYPE(np->n_type)) {
	case N_ABS:
		return(ext? 'A' : 'a');
	case N_BSS:
		return(ext? 'B' : 'b');
	case N_COMM:
		return(ext? 'C' : 'c');
	case N_DATA:
		return(ext? 'D' : 'd');
	case N_FN:
		/* NOTE: N_FN == N_WARNING,
		 * in this case, the N_EXT bit is to considered as
		 * part of the symbol's type itself.
		 */
		return(ext? 'F' : 'W');
	case N_TEXT:
		return(ext? 'T' : 't');
	case N_INDR:
		return(ext? 'I' : 'i');
	case N_SIZE:
		return(ext? 'S' : 's');
	case N_UNDF:
		return(ext? 'U' : 'u');
	}
	return('?');
}

int
fname(const void *a0, const void *b0)
{
	struct nlist * const *a = a0, * const *b = b0;

	return(strcmp((*a)->n_un.n_name, (*b)->n_un.n_name));
}

int
rname(const void *a0, const void *b0)
{
	struct nlist * const *a = a0, * const *b = b0;

	return(strcmp((*b)->n_un.n_name, (*a)->n_un.n_name));
}

int
value(const void *a0, const void *b0)
{
	struct nlist * const *a = a0, * const *b = b0;

	if (SYMBOL_TYPE((*a)->n_type) == N_UNDF)
		if (SYMBOL_TYPE((*b)->n_type) == N_UNDF)
			return(0);
		else
			return(-1);
	else if (SYMBOL_TYPE((*b)->n_type) == N_UNDF)
		return(1);
	if (rev) {
		if ((*a)->n_value == (*b)->n_value)
			return(rname(a0, b0));
		return((*b)->n_value > (*a)->n_value ? 1 : -1);
	} else {
		if ((*a)->n_value == (*b)->n_value)
			return(fname(a0, b0));
		return((*a)->n_value > (*b)->n_value ? 1 : -1);
	}
}

#define CPPFILT	"/usr/bin/c++filt"

void
pipe2cppfilt(void)
{
	int pip[2];
	char *argv[2];

	argv[0] = "c++filt";
	argv[1] = NULL;

	if (pipe(pip) == -1)
		err(1, "pipe");
	switch(fork()) {
	case -1:
		err(1, "fork");
	default:
		dup2(pip[0], 0);
		close(pip[0]);
		close(pip[1]);
		execve(CPPFILT, argv, NULL);
		err(1, "execve");
	case 0:
		dup2(pip[1], 1);
		close(pip[1]);
		close(pip[0]);
	}
}

void
usage(void)
{
	extern char *__progname;

	if (issize)
		fprintf(stderr, "usage: %s [-tw] [file ...]\n", __progname);
	else
		fprintf(stderr, "usage: %s [-aCegnoprsuVw] [file ...]\n",
		    __progname);
	exit(1);
}
