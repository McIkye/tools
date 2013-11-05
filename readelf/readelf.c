/*
 * Copyright (c) 2012-2013 Michael Shalayeff
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
static const char rcsid[] =
    "$ABSD: readelf.c,v 1.8 2013/11/05 17:25:29 mickey Exp $";
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

const char elf_relalpha[][16] = {
	"NONE",
	"REFLONG",
	"REFQUAD",
	"GPREL32",
	"LITERAL",
	"LITUSE",
	"GPDISP",
	"BRADDR",
	"HINT",
	"SREL16",
	"SREL32",
	"SREL64",
	"OP_PUSH",
	"OP_STORE",
	"OP_PSUB",
	"OP_PRSHIFT",
	"GPVALUE",
	"GPRELHIGH",
	"GPRELLOW",
	"IMM_GP_16",
	"IMM_GP_HI32",
	"IMM_SCN_HI32",
	"IMM_BRELOC_HI32",
	"IMM_LO32",
	"COPY",
	"GLOB_DAT",
	"JMP_SLOT",
	"RELATIVE",
};

const char elf_relamd64[][16] = {
	"NONE",
	"64",
	"PC32",
	"GOT32",
	"PLT32",
	"COPY",
	"GLOB_DAT",
	"JUMP_SLOT",
	"RELATIVE",
	"GOTPCREL",
	"32",
	"32S",
	"16",
	"PC16",
	"8",
	"PC8",
	"DPTMOD64",
	"DTPOFF64",
	"TPOFF64",
	"TLSGD",
	"TLSLD",
	"DTPOFF32",
	"GOTTPOFF",
	"TPOFF32",
};

const char elf_relarm[][16] = {
	"NONE",
	"PC24",
	"ABS32",
	"REL32",
	"PC13",
	"ABS16",
	"ABS12",
	"THM_ABS5",
	"ABS8",
	"SBREL32",
	"THM_PC22",
	"THM_PC8",
	"AMP_VCALL9",
	"SWI24",
	"THM_SWI8",
	"XPC25",
	"THM_XPC22",
	"", "", "",
	"COPY",
	"GLOB_DAT",
	"JUMP_SLOT",
	"RELATIVE",
	"GOTOFF",
	"GOTPC",
	"GOT32",
	"PLT32",
	"", "", "", "",
	"ALU_PCREL_7_0",
	"ALU_PCREL_15_8",
	"ALU_PCREL_23_15",
	"ALU_SBREL_11_0",
	"ALU_SBREL_19_12",
	"ALU_SBREL_27_20",
	"", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"GNU_VTENTRY",
	"GNU_VTINHERIT",
	"THM_PC11",
	"THM_PC9",
	"", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "",
	"RXPC25",
	"RSBREL32",
	"THM_RPC22",
	"RREL32",
	"RABS32",
	"RPC24",
	"RBASE",
};

const char elf_rel386[][16] = {
	"NONE",
	"32",
	"PC32",
	"GOT32",
	"PLT32",
	"COPY",
	"GLOB_DAT",
	"JUMP_SLOT",
	"RELATIVE",
	"GOTOFF",
	"GOTPC",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"16",
	"PC16",
	"8",
	"PC8",
};

const char elf_relhppa[][16] = {
	"NONE",
	"DIR32",
	"DIR21L",
	"DIR17R",
	"DIR17F",
	"",
	"DIR14R",
	"",
	"PCREL12F",
	"PCREL32",
	"PCREL21L",
	"PCREL17R",
	"PCREL17F",
	"PCREL17C",
	"PCREL14R",
	"", "", "",
	"DPREL21L",
	"DPREL14WR",
	"DPREL14DR",
	"",
	"DPREL14R",
	"", "", "",
	"GPREL21L",
	"", "", "",
	"GPREL14R",
	"", "", "",
	"LTOFF21L",
	"", "", "",
	"LTOFF14R",
	"LTOFF14F",
	"SETBASE",
	"SECREL32",
	"BASEREL21L",
	"BASEREL17R",
	"", "",
	"BASEREL14R",
	"",
	"SEGBASE",
	"SEGREL32",
	"PLTOFF21L",
	"", "", "",
	"PLTOFF14R",
	"PLTOFF14F",
	"",
	"LTOFF_FPTR32",
	"LTOFF_FPTR21L",
	"", "", "",
	"LTOFF_FPTR14R",
	"",
	"FPTR64",
	"PLABEL32",
	"", "", "", "", "", "",
	"PCREL64",
	"PCREL22C",
	"PCREL22F",
	"PCREL14WR",
	"PCREL14DR",
	"PCREL16F",
	"PCREL16WF",
	"PCREL16DF",
	"DIR64",
	"", "",
	"DIR14WR",
	"DIR14DR",
	"DIR16F",
	"DIR16WF",
	"DIR16DF",
	"GPREL64",
	"", "",
	"GPREL14WR",
	"GPREL14DR",
	"GPREL16F",
	"GPREL16WF",
	"GPREL16DF",
	"LTOFF64",
	"", "",
	"LTOFF14WR",
	"LTOFF14DR",
	"LTOFF16F",
	"LTOFF16WF",
	"LTOFF16DF",
	"SECREL64",
	"", "",
	"BASEREL14WR",
	"BASEREL14DR",
	"", "", "",
	"SEGREL64",
	"", "",
	"PLTOFF14WR",
	"PLTOFF14DR",
	"PLTOFF16F",
	"PLTOFF16WF",
	"PLTOFF16DF",
	"LTOFF_FPTR64",
	"", "",
	"LTOFF_FPTR14WR",
	"LTOFF_FPTR14DR",
	"LTOFF_FPTR16F",
	"LTOFF_FPTR16WF",
	"LTOFF_FPTR16DF",
	"COPY",
	"IPLT",
	"EPLT",
	"GDATA",
	"JMPSLOT",
	"RELATIVE"
};

const char elf_relmips[][16] = {
	"NONE",
	"16",
	"32",
	"REL32",
	"26",
	"HI16",
	"LO16",
	"GPREL16",
	"LITERAL",
	"GOT16",
	"PC16",
	"CALL16",
	"GPREL32",
	"", "", "",
	"SHIFT5",
	"SHIFT6",
	"64",
	"GOT_DISP",
	"GOT_BASE",
	"GOT_OFST",
	"GOT_HI16",
	"GOT_LO16",
	"GOT_SUB",
	"INSERT_A",
	"INSERT_B",
	"DELETE",
	"HIGHER",
	"HIGHEST",
	"CALL_HI16",
	"CALL_LO16",
	"SCN_DISP",
	"REL16",
	"ADD_IMM",
	"PJUMP",
	"RELGOT",
	"JALR",
	"", "",
	"", "", "", "", "", "", "", "", "", "",	/* 40 */
	"", "", "", "", "", "", "", "", "", "",	/* 50 */
	"", "", "", "", "", "", "", "", "", "",	/* 60 */
	"", "", "", "", "", "", "", "", "", "",	/* 70 */
	"", "", "", "", "", "", "", "", "", "",	/* 80 */
	"", "", "", "", "", "", "", "", "", "",	/* 90 */
	"16_26",
	"16_GPREL",
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",	/* 110 */
	"", "", "", "", "", "",
	"QNX_COPY",
	"", "", "",
	"", "", "", "", "", "", "", "", "", "",	/* 130 */
	"", "", "", "", "", "", "", "", "", "",	/* 140 */
	"", "", "", "", "", "", "", "", "", "",	/* 150 */
	"", "", "", "", "", "", "", "", "", "",	/* 160 */
	"", "", "", "", "", "", "", "", "", "",	/* 170 */
	"", "", "", "", "", "", "", "", "", "",	/* 180 */
	"", "", "", "", "", "", "", "", "", "",	/* 190 */
	"", "", "", "", "", "", "", "", "", "",	/* 200 */
	"", "", "", "", "", "", "", "", "", "",	/* 210 */
	"", "", "", "", "", "", "", "", "", "",	/* 220 */
	"", "", "", "", "", "", "", "", "", "",	/* 230 */
	"", "", "", "", "", "", "", "",
	"PC32",
	"",
	"GNU_REL16_S2",
	"", "",
	"GNU_VTINHERIT",
	"GNU_VTENTRY"
};

const char elf_relppc[][16] = {
	"NONE",
	"32",
	"24",
	"16",
	"16_LO",
	"16_HI",
	"16_HA",
	"14",
	"14_TAKEN",
	"14_NTAKEN",
	"REL24",
	"REL14",
	"REL14_TAKEN",
	"REL14_NTAKEN",
	"GOT16",
	"GOT16_LO",
	"GOT16_HI",
	"GOT16_HA",
	"PLT24",
	"COPY",
	"GLOB_DAT",
	"JMP_SLOT",
	"RELATIVE",
	"LOCAL24PC",
	"U32",
	"U16",
	"REL32",
	"PLT32",
	"PLTREL32",
	"PLT16_LO",
	"PLT16_HI",
	"PLT16_HA",
	"SDAREL",
};

const char elf_relish[][16] = {
	"NONE",
	"DIR32",
	"REL32",
	"DIR8WPN",
	"IND12W",
	"DIR8WPL",
	"DIR8WPZ",
	"DIR8BP",
	"DIR8W",
	"DIR8L",
	"LOOP_START",
	"LOOP_END",
	"", "", "", "", "", "", "", "", "", "",
	"GNU_VTINHERIT",
	"GNU_VTENTRY",
	"SWITCH8",
	"SWITCH16",
	"SWITCH32",
	"USES",
	"COUNT",
	"ALIGN",
	"CODE",
	"DATA",
	"LABEL",
	"DIR16",
	"DIR8",
	"DIR8UL",
	"DIR8UW",
	"DIR8U",
	"DIR8SW",
	"DIR8S",
	"DIR4UL",
	"DIR4UW",
	"DIR4U",
	"PSHA",
	"PSHL",
	"DIR5U",
	"DIR6U",
	"DIR6S",
	"DIR10S	",
	"DIR10SW",
	"DIR10SL",
	"DIR10SQ",
	"",
	"DIR16S",
	"", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "",
	"TLS_GD_32",
	"TLS_LD_32",
	"TLS_LDO_32",
	"TLS_IE_32",
	"TLS_LE_32",
	"TLS_DTPMOD32",
	"TLS_DTPOFF32",
	"TLS_TPOFF32",
	"", "", "", "", "", "", "", "",
	"GOT32",
	"PLT32",
	"COPY",
	"GLOB_DAT",
	"JMP_SLOT",
	"RELATIVE",
	"GOTOFF",
	"GOTPC",
	"GOTPLT32",
	"GOT_LOW16",
	"GOT_MEDLOW16",
	"GOT_MEDHI16",
	"GOT_HI16",
	"GOTPLT_LOW16",
	"GOTPLT_MEDLOW16",
	"GOTPLT_MEDHI16",
	"GOTPLT_HI16",
	"PLT_LOW16",
	"PLT_MEDLOW16",
	"PLT_MEDHI16",
	"PLT_HI16",
	"GOTOFF_LOW16",
	"GOTOFF_MEDLOW16",
	"GOTOFF_MEDHI16",
	"GOTOFF_HI16",
	"GOTPC_LOW16",
	"GOTPC_MEDLOW16",
	"GOTPC_MEDHI16",
	"GOTPC_HI16",
	"GOT10BY4",
	"GOTPLT10BY4",
	"GOT10BY8",
	"GOTPLT10BY8",
	"COPY64",
	"GLOB_DAT64",
	"JMP_SLOT64",
	"RELATIVE64",
	"", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "",
	"SHMEDIA_CODE",
	"PT_16",
	"IMMS16",
	"IMMU16",
	"IMM_LOW16",
	"IMM_LOW16_PC",
	"IMM_MEDLOW16",
	"IMM_MEDLOW16_PC",
	"IMM_MEDHI16",
	"IMM_MEDHI16_PC",
	"IMM_HI16",
	"IMM_HI16_PC",
	"64",
	"64_PC"
};

const char elf_relsparc[][16] = {
	"NONE",
	"", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "",
	"COPY",
	"GLOB_DAT",
	"JMP_SLOT",
	"RELATIVE",
	"", "", "", "", "", "", "", "", "",
	"64",
	"", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "",
	"UA64"
};

const char *
elf_reltype(int m, int r)
{
	static char buf[32];
	const char *rr, *p;
	int n = 0;

	switch (m) {
	case EM_SPARCV9:
		p = "SPARC64";
	case EM_SPARC:
		if (m == EM_SPARC)
			p = "SPARC";
		rr = elf_relsparc[r];
		n = sizeof elf_relsparc / sizeof elf_relsparc[0];
		break;

	case EM_ALPHA:
		p = "ALPHA";
		rr = elf_relalpha[r];
		n = sizeof elf_relalpha / sizeof elf_relalpha[0];
		break;

	case EM_ARM:
		p = "ARM";
		rr = elf_relarm[r];
		n = sizeof elf_relarm / sizeof elf_relarm[0];
		break;

	case EM_AMD64:
		p = "AMD64";
		rr = elf_relamd64[r];
		n = sizeof elf_relamd64 / sizeof elf_relamd64[0];
		break;

	case EM_386:
		p = "I386";
		rr = elf_rel386[r];
		n = sizeof elf_rel386 / sizeof elf_rel386[0];
		break;

	case EM_PPC:
		p = "PPC";
		rr = elf_relppc[r];
		n = sizeof elf_relppc / sizeof elf_relppc[0];
		break;

	case EM_PARISC:
		p = "PARISC";
		rr = elf_relhppa[r];
		n = sizeof elf_relhppa / sizeof elf_relhppa[0];
		break;

	case EM_SH:
		p = "SH";
		rr = elf_relish[r];
		n = sizeof elf_relish / sizeof elf_relish[0];
		break;

	case EM_MIPS:
		p = "MIPS";
		rr = elf_relmips[r];
		n = sizeof elf_relmips / sizeof elf_relmips[0];
		break;

	default:
		snprintf(buf, sizeof buf, "0x%x", r);
		return buf;
	}

	if (r >= 0 && r < n && *rr != '\0')
		snprintf(buf, sizeof buf, "R_%s_%s", p, rr);
	else
		snprintf(buf, sizeof buf, "R_0x%x_0x%x", m, r);

	return buf;
}
