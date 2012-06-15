/*
 * Copyright (c) 2012 Michael Shalayeff
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <getopt.h>
#include <elf_abi.h>
#include <elfuncs.h>

#ifndef lint
static const char rcsid[] = "$ABSD: readelf.c,v 1.4 2012/06/15 16:35:15 mickey Exp $";
#endif

#include "readelf.h"

#define	OPTSTRING	"aAdDehIlnsSruvVw::Wx:"
const struct option longopts[] = {
	{ "all",		no_argument,		0, 'a' },
	{ "arch-specific",	no_argument,		0, 'A' },
	{ "debug-dump",		required_argument,	0, 'w' },
	{ "dynamic",		no_argument,		0, 'd' },
	{ "file-header",	no_argument,		0, 'h' },
	{ "headers",		no_argument,		0, 'e' },
	{ "hex-dump",		required_argument,	0, 'x' },
	{ "histogram",		no_argument,		0, 'I' },
	{ "notes",		no_argument,		0, 'n' },
	{ "program-headers",	no_argument,		0, 'l' },
	{ "relocs",		no_argument,		0, 'r' },
	{ "sections-headers",	no_argument,		0, 'S' },
	{ "segments",		no_argument,		0, 'l' },
	{ "sections",		no_argument,		0, 'S' },
	{ "symbols",		no_argument,		0, 's' },
	{ "syms",		no_argument,		0, 's' },
	{ "unwind",		no_argument,		0, 'u' },
	{ "use-dynamic",	no_argument,		0, 'D' },
	{ "verify",		no_argument,		0, 'v' },
	{ "version-info",	no_argument,		0, 'V' },
	{ "wide",		no_argument,		0, 'W' },
};

int opts;
int nsyms;
struct sym *symidx;

void
usage(void)
{
	extern const char *__progname;

	fprintf(stderr,
	    "usage: %s -[aAdDehIlnsSruVW] [-x <number>] [-w[afFilmoprs]]\n",
	    __progname);

	exit(1);
}

int
main(int argc, char **argv)
{
	union {
		Elf32_Ehdr elf32;
		Elf64_Ehdr elf64;
	} eh;
	FILE *fp;
	void *phs, *shs, *shn;
	int ch, li, errs;

	while ((ch = getopt_long(argc, argv, OPTSTRING, longopts, &li)) != -1)
		switch (ch) {
		case 0:
			break;

		case 'a':
			opts |= ALL;
			break;

		case 'A':
			opts |= ARCHS;
			break;

		case 'd':
			opts |= DYNS;
			break;

		case 'D':
			opts |= USEDYN;
			break;

		case 'e':
			opts |= HDRS;
			break;

		case 'h':
			opts |= FILEH;
			break;

		case 'I':
			opts |= HIST;
			break;

		case 'l':
			opts |= PROGH;
			break;

		case 'n':
			opts |= NOTES;
			break;

		case 's':
			opts |= SYMS;
			break;

		case 'S':
			opts |= SECH;
			break;

		case 'r':
			opts |= RELOCS;
			break;

		case 'u':
			opts |= UNWIND;
			break;

		case 'v':
			opts |= VERIFY;
			break;

		case 'V':
			opts |= VERS;
			break;

		case 'w':
			break;

		case 'W':
			opts |= WIDE;
			break;

		case 'x':
			break;

		default:
			usage();
			break;
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		errx(1, "no input files");

/* TODO check valid options combination */

	for (errs = 0; argc--; argv++) {
		if (!(fp = fopen(*argv, "r")))
			err(1, "fopen: %s", *argv);

		if (fread(&eh, sizeof eh, 1, fp) != 1)
			err(1, "fread header: %s", *argv);

/* TODO handle archives */

		if (!((!elf32_chk_header(&eh.elf32) &&
		    !elf32_gethdrs(fp, *argv, 0, &eh.elf32,&phs,&shs,&shn)) ||
		    (!elf64_chk_header(&eh.elf64) &&
		    !elf64_gethdrs(fp, *argv, 0, &eh.elf64,&phs,&shs,&shn)))) {
			warnx("%s: invalid object", *argv);
			errs++;
			continue;
		}

		if (opts & VERIFY) {
			if (eh.elf32.e_ident[EI_CLASS] == ELFCLASS32)
				elf32_verify(&eh.elf32, phs, shs, shn);
			else
				elf64_verify(&eh.elf64, phs, shs, shn);
		}

		if (opts & FILEH) {
			if (eh.elf32.e_ident[EI_CLASS] == ELFCLASS32)
				elf32_prhdr(&eh.elf32);
			else
				elf64_prhdr(&eh.elf64);
		}

		if (opts & PROGH) {
			if (eh.elf32.e_ident[EI_CLASS] == ELFCLASS32)
				elf32_prsegs(&eh.elf32, phs);
			else
				elf64_prsegs(&eh.elf64, phs);
		}

		if (opts & SECH) {
			if (eh.elf32.e_ident[EI_CLASS] == ELFCLASS32)
				elf32_prsechs(&eh.elf32, shs, shn);
			else
				elf64_prsechs(&eh.elf64, shs, shn);
		}

		if (opts & SYMS) {
			if (eh.elf32.e_ident[EI_CLASS] == ELFCLASS32)
				elf32_prsyms(&eh.elf32, shs, shn);
			else
				elf64_prsyms(&eh.elf64, shs, shn);
		}

		if (opts & RELOCS) {
			if (eh.elf32.e_ident[EI_CLASS] == ELFCLASS32)
				elf32_prels(fp, *argv, 0, &eh.elf32, shs, shn);
			else
				elf64_prels(fp, *argv, 0, &eh.elf64, shs, shn);
		}

		if (argc) {
			free(symidx);
			symidx = NULL;
			nsyms = 0;

			free(shn);
			free(shs);
			free(phs);
		}
	}

	return errs;
}

const char *elf_osabis[] = {
	"UNIX - System V",
	"HPU/UX",
	"NetBSD",
	"GNU/Linux",
	"GNU/Hurd",
	"IA32",
	"Solaris",
	"Monterey",
	"IRIX",
	"FreeBSD",
	"TRU64 UNIX",
	"Novell Modesto",
	"OpenBSD",
	"OpenVMS"
};

const char *
elf_osabi(int os)
{
	static char buf[32];

	if (os >= 0 && os < sizeof elf_osabis / sizeof elf_osabis[0])
		return elf_osabis[os];

	snprintf(buf, sizeof buf, "0x%x", os);
	return buf;
}

const char *elf_types[] = {
	"None",
	"REL",
	"EXEC",
	"DYN",
	"CORE"
};

const char *
elf_type(int ty)
{
	static char buf[32];

	if (ty >= 0 && ty < sizeof elf_types / sizeof elf_types[0])
		return elf_types[ty];

	snprintf(buf, sizeof buf, "0x%x", ty);
	return buf;
}

const struct {
	int id;
	const char *name;
} elf_machines[] = {
	{ 1,	"AT&T WE 32100" },
	{ 2,	"SPARC" },
	{ 3,	"Intel 80386" },
	{ 4,	"Motorola 68000" },
	{ 5,	"Motorola 88000" },
	{ 6,	"Intel 80486" },
	{ 7,	"Intel 80860" },
	{ 8,	"MIPS R3000" },
	{ 10,	"MIPS R4000" },
	{ 11,	"SPARC v9 64bit" },
	{ 15,	"HP PA-RISC" },
	{ 18,	"SPARC 32+" },
	{ 19,	"Intel 960" },
	{ 20,	"PowerPC" },
	{ 21,	"PowerPC 64bit" },
	{ 22,	"IBM S/390" },
	{ 40,	"ARM" },
	{ 41,	"ALPHA" },
	{ 42,	"Super-H" },
	{ 43,	"SPARC v9" },
	{ 62,	"AMD64" },
	{ 64,	"DEC PDP-10" },
	{ 65,	"DEC PDP-11" },
	{ 75,	"DEC VAX" },
	{ 80,	"MMIX" },
	{ 83,	"AVR" },
	{ 92,	"OpenRISC" },
	{ 185,	"AVR32" },
	{ 189,	"MicroBlaze" }
};

const char *
elf_machine(int ma)
{
	static char buf[32];
	int i;

	for (i = 0; i < sizeof elf_machines / sizeof elf_machines[0]; i++)
		if (ma == elf_machines[i].id)
			return elf_machines[i].name;

	snprintf(buf, sizeof buf, "machine %d", ma);
	return buf;
}

const char *
elf_pflags(int f)
{
	static char buf[8];

	buf[0] = f & PF_R? 'R' : ' ';
	buf[1] = f & PF_W? 'W' : ' ';
	buf[2] = f & PF_X? 'X' : ' ';

	return buf;
}

const char *elf_phtypes[] = {
	"NULL",
	"LOAD",
	"DYNAMIC",
	"INTERP",
	"NOTE",
	"SHLIB",
	"PHDR",
};

const char *
elf_phtype(int ph)
{
	static char buf[32];

	if (ph >= 0 && ph < sizeof elf_phtypes / sizeof elf_phtypes[0])
		return elf_phtypes[ph];

	snprintf(buf, sizeof buf, "0x%x", ph);
	return buf;
}

const char *
elf_sflags(int f)
{
	static char buf[8];

	buf[0] = f & SHF_WRITE? 'W' : ' ';
	buf[1] = f & SHF_ALLOC? 'A' : ' ';
	buf[2] = f & SHF_EXECINSTR? 'X' : ' ';

	return buf;
}

const char *elf_shtypes[] = {
	"NULL",
	"PROGBITS",
	"SYMTAB",
	"STRTAB",
	"RELA",
	"HASH",
	"DYNAMIC",
	"NOTE",
	"NOBITS",
	"REL",
	"SHLIB",
	"DYNSYM",
};

const char *
elf_shtype(int sh)
{
	static char buf[32];

	if (sh >= 0 && sh < sizeof elf_shtypes / sizeof elf_shtypes[0])
		return elf_shtypes[sh];

	snprintf(buf, sizeof buf, "0x%x", sh);
	return buf;
}

const char *
elf_shndx(int shn)
{
	static char buf[32];

	if (shn == SHN_UNDEF)
		return "UND";

	if (shn == SHN_ABS)
		return "ABS";

	if (shn == SHN_COMMON)
		return "COM";

	snprintf(buf, sizeof buf, "%d", shn);
	return buf;
}

const char *elf_symtypes[] = {
	"NOTYPE",
	"OBJECT",
	"FUNC",
	"SECTION",
	"FILE",
};

const char *
elf_symtype(int st)
{
	static char buf[32];

	if (st >= 0 && st < sizeof elf_symtypes / sizeof elf_symtypes[0])
		return elf_symtypes[st];

	snprintf(buf, sizeof buf, "0x%x", st);
	return buf;
}

const char *elf_symbinds[] = {
	"LOCAL",
	"GLOBAL",
	"WEAK",
};

const char *
elf_symbind(int sb)
{
	static char buf[32];

	if (sb >= 0 && sb < sizeof elf_symbinds / sizeof elf_symbinds[0])
		return elf_symbinds[sb];

	snprintf(buf, sizeof buf, "0x%x", sb);
	return buf;
}

const char *elf_rel386[] = {
	"NONE",
	"I386_32",
	"I386_PC32",
	"I386_GOT32",
	"I386_PLT32",
	"I386_COPY",
	"I386_GLOB_DAT",
	"I386_JUMP_SLOT",
	"I386_RELATIVE",
	"I386_GOTOFF",
	"I386_GOTPC",
	"#11",
	"#12",
	"#13",
	"#14",
	"#15",
	"#16",
	"#17",
	"#18",
	"#19",
	"I386_16",
	"I386_PC16",
	"I386_8",
	"I386_PC8",
};

const char *
elf_reltype(int m, int r)
{
	static char buf[32];
	const char **b;
	int n;

	switch (m) {
	case EM_386:
		b = elf_rel386;
		n = sizeof elf_rel386 / sizeof elf_rel386[0];
		break;
	default:
		snprintf(buf, sizeof buf, "0x%x", r);
		return buf;
	}

	if (r >= 0 && r < n)
		return b[r];

	snprintf(buf, sizeof buf, "0x%x", r);
	return buf;
}
