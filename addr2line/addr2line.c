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
    "$ABSD: addr2line.c,v 1.4 2014/08/08 16:39:12 mickey Exp $";
#endif

#include <sys/param.h>
#include <a.out.h>
#include <elf_abi.h>
#include <stab.h>
#include <dwarf.h>
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
#include <libgen.h>
#include <elfuncs.h>

#include "byte.c"

#define	A2LFUNAME	1
#define	A2LBASENAME	2

void usage(void);
int addr2line(const char *, FILE *, long long, const char **, const char **,
    const char **, int *);
void a2lprintf(const char *, const char *, const char *, int, int);

/* a funky nlist overload for reading 32bit a.out on 64bit toys */
/* stolen from nm.c */
struct nlist32 {
	u_int32_t	strx;
	u_int8_t	type;
	u_int8_t	other;
	u_int16_t	desc;
	u_int32_t	value;
} __packed;

int
main(int argc, char *argv[])
{
	extern int optind;
	const char *fn;
	FILE *fp;
	char *ep;
	const char *dir, *fname, *funame;
	long long ptr;
	int ch, flags, ln;

	fn = "a.out";
	flags = 0;
	while ((ch = getopt(argc, argv, "e:fs")) != -1)
		switch (ch) {
		case 'e':
			fn = optarg;
			break;

		case 'f':
			flags |= A2LFUNAME;
			break;

		case 's':
			flags |= A2LBASENAME;
			break;

		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (!(fp = fopen(fn, "r")))
		err(1, "fopen: %s", fn);

	if (*argv)
		for (; *argv; argv++) {
			errno = 0;
			ptr = strtoull(*argv, &ep, 0);
			if (*argv[0] == '\0' || *ep != '\0')
				errx(1, "\'%s\' is ain't no number", *argv);
			if (errno == ERANGE && ptr == ULLONG_MAX)
				errx(1, "\'%s\' address's out of there", *argv);

			if (!addr2line(fn, fp, ptr, &dir, &fname, &funame, &ln))
				a2lprintf(dir, fname, funame, ln, flags);
		}
	else
		while (scanf("%lld", &ptr) != EOF)
			if (!addr2line(fn, fp, ptr, &dir, &fname, &funame, &ln))
				a2lprintf(dir, fname, funame, ln, flags);

	return 0;
}

void
a2lprintf(const char *dir, const char *fname, const char *funame, int ln, int flags)
{
	if (!(flags & A2LBASENAME))
		printf("%s/", dir);

	printf("%s:", fname);

	if (flags & A2LFUNAME)
		printf("%s:", funame);

	printf("%d\n", ln);
}

void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s [-fs] [-e a.out] [addr ...]\n", __progname);
	exit(1);
}

int
addr2line(const char *name, FILE *fp, long long addr, const char **pdir,
    const char **pfn, const char **pfun, int *pln)
{
	union hdr {
		struct exec aout;
		Elf32_Ehdr elf32;
		Elf64_Ehdr elf64;
	} head;
	struct dwarf_nebula *dn;
	size_t bytes;
	int rv;

	bytes = fread((char *)&head, 1, sizeof(head), fp);
	if (bytes < sizeof(head))
		if (bytes < sizeof(head.aout) || IS_ELF(head.elf32)) {
			warnx("%s: bad format", name);
			fclose(fp);
			return 1;
		}

	if (IS_ELF(head.elf32) && head.elf32.e_ident[EI_CLASS] == ELFCLASS32) {

		elf32_fix_header(&head.elf32);
		if (elf32_chk_header(&head.elf32) == -1)
			return 1;

		if (!(dn = elf32_dwarfnebula(name, fp, 0, &head.elf32,
		    ELF_DWARF_LINES | ELF_DWARF_NAMES)))
			return 1;

		rv = dwarf_addr2line(addr, dn, pdir, pfn, pln);

	} else if (IS_ELF(head.elf64) &&
	    head.elf64.e_ident[EI_CLASS] == ELFCLASS64) {

		elf64_fix_header(&head.elf64);
		if (elf64_chk_header(&head.elf64) == -1)
			return 1;

		if (!(dn = elf64_dwarfnebula(name, fp, 0, &head.elf64,
		    ELF_DWARF_LINES | ELF_DWARF_NAMES)))
			return 1;

		rv = dwarf_addr2line(addr, dn, pdir, pfn, pln);

	} else if (BAD_OBJECT(head.aout)) {
		warnx("%s: bad format", name);
		return 1;
	} else {
		struct nlist nl;
		char *strtab;
		u_int32_t w;
		int nsym;

		fix_header_order(&head.aout);
		/* stop if the object file contains no symbol table */
		if (!head.aout.a_syms) {
			warnx("%s: no name list", name);
			return 1;
		}

		if (fseeko(fp, head.aout.a_syms + N_SYMOFF(head.aout),
		    SEEK_SET)) {
			warn("%s: fseeko", name);
			return 1;
		}

		/* get the string table size */
		if (fread(&w, sizeof(w), (size_t)1, fp) != 1) {
			warnx("%s: cannot read stab size", name);
			return 1;
		}

		w = fix_32_order(w, N_GETMID(head.aout));
		if (!(strtab = malloc(w))) {
			warn("malloc: strtab (%u bytes)", w);
			return 1;
		}

		if (fread(strtab, w, 1, fp) != 1) {
			warn("%s: read stab", name);
			return 1;
		}

		/* seek back to the symtab */
		if (fseeko(fp, N_SYMOFF(head.aout), SEEK_SET)) {
			warn("%s: fseeko", name);
			return 1;
		}

		rv = 1;
#ifdef __LP64__
		for (nsym = head.aout.a_syms / sizeof(struct nlist32);
		    nsym--; ) {
			struct nlist32 nl32;

			if (fread(&nl32, sizeof(nl32), 1, fp) != 1) {
				warnx("%s: cannot read symbol table", name);
				free(strtab);
				return 1;
			}

			nl.n_type = nl32.type;
			nl.n_other = nl32.other;
			if (byte_sex(N_GETMID(head.aout)) != BYTE_ORDER) {
				nl.n_un.n_strx = swap32(nl32.strx);
				nl.n_desc = swap16(nl32.desc);
				nl.n_value = swap32(nl32.value);
			} else {
				nl.n_un.n_strx = nl32.strx;
				nl.n_desc = nl32.desc;
				nl.n_value = nl32.value;
			}
#else
		for (nsym = head.aout.a_syms / sizeof(nl); nsym--; ) {

			if (fread(&nl, sizeof(nl), 1, fp) != 1) {
				warnx("%s: cannot read symbol table", name);
				free(strtab);
				return 1;
			}
			if (byte_sex(N_GETMID(head.aout)) != BYTE_ORDER)
				swap_nlist(&nl);
#endif
			nl.n_un.n_name = strtab + nl.n_un.n_strx;

			switch (nl.n_type) {
			case N_SO:
			case N_SOL:
				if (nl.n_value <= addr)
					*pfn = nl.n_un.n_name;
				break;

			case N_TEXT:
				if (nl.n_un.n_name[0] != '_')
					break;
				/* FALLTHROUGH */
			case N_FUN:
			case N_ENTRY:
				if (nl.n_value <= addr)
					*pfun = nl.n_un.n_name;
				break;

			case N_SLINE:
				if (nl.n_value <= addr)
					*pln = nl.n_desc;
				break;
			}
		}

		if (*pln > 0 && *pfn)
			rv = 0;

		free(strtab);
	}

	return rv;
}
