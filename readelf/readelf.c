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

const char *const elf_osabis[] = {
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

const char *const elf_types[] = {
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

const char *const elf_phtypes[] = {
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

const char *const elf_shtypes[] = {
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

const char *const elf_symtypes[] = {
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

const char *const elf_symbinds[] = {
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

const char *const elf_relalpha[] = {
	"NONE",
	"ALPHA_REFLONG",
	"ALPHA_REFQUAD",
	"ALPHA_GPREL32",
	"ALPHA_LITERAL",
	"ALPHA_LITUSE",
	"ALPHA_GPDISP",
	"ALPHA_BRADDR",
	"ALPHA_HINT",
	"ALPHA_SREL16",
	"ALPHA_SREL32",
	"ALPHA_SREL64",
	"ALPHA_OP_PUSH",
	"ALPHA_OP_STORE",
	"ALPHA_OP_PSUB",
	"ALPHA_OP_PRSHIFT",
	"ALPHA_GPVALUE",
	"ALPHA_GPRELHIGH",
	"ALPHA_GPRELLOW",
	"ALPHA_IMMED_GP_16",
	"ALPHA_IMMED_GP_HI32",
	"ALPHA_IMMED_SCN_HI32",
	"ALPHA_IMMED_BRELOC_HI32",
	"ALPHA_IMMED_LO32",
	"ALPHA_COPY",
	"ALPHA_GLOB_DAT",
	"ALPHA_JMP_SLOT",
	"ALPHA_RELATIVE",
};

const char *const elf_relamd64[] = {
	"NONE",
	"AMD64_64",
	"AMD64_PC32",
	"AMD64_GOT32",
	"AMD64_PLT32",
	"AMD64_COPY",
	"AMD64_GLOB_DAT",
	"AMD64_JUMP_SLOT",
	"AMD64_RELATIVE",
	"AMD64_GOTPCREL",
	"AMD64_32",
	"AMD64_32S",
	"AMD64_16",
	"AMD64_PC16",
	"AMD64_8",
	"AMD64_PC8",
	"AMD64_DPTMOD64",
	"AMD64_DTPOFF64",
	"AMD64_TPOFF64",
	"AMD64_TLSGD",
	"AMD64_TLSLD",
	"AMD64_DTPOFF32",
	"AMD64_GOTTPOFF",
	"AMD64_TPOFF32",
};

const char *const elf_relarm[] = {
	"NONE",
	"ARM_PC24",
	"ARM_ABS32",
	"ARM_REL32",
	"ARM_PC13",
	"ARM_ABS16",
	"ARM_ABS12",
	"ARM_THM_ABS5",
	"ARM_ABS8",
	"ARM_SBREL32",
	"ARM_THM_PC22",
	"ARM_THM_PC8",
	"ARM_AMP_VCALL9",
	"ARM_SWI24",
	"ARM_THM_SWI8",
	"ARM_XPC25",
	"ARM_THM_XPC22",
	"ARM #17",
	"ARM #18",
	"ARM #19",
	"ARM_COPY",
	"ARM_GLOB_DAT",
	"ARM_JUMP_SLOT",
	"ARM_RELATIVE",
	"ARM_GOTOFF",
	"ARM_GOTPC",
	"ARM_GOT32",
	"ARM_PLT32",
	"ARM #28",
	"ARM #29",
	"ARM #30",
	"ARM #31",
	"ARM_ALU_PCREL_7_0",
	"ARM_ALU_PCREL_15_8",
	"ARM_ALU_PCREL_23_15",
	"ARM_ALU_SBREL_11_0",
	"ARM_ALU_SBREL_19_12",
	"ARM_ALU_SBREL_27_20",
	"ARM #38",
	"ARM #39",
	"ARM #40", "ARM #41", "ARM #42", "ARM #43", "ARM #44",
	"ARM #45", "ARM #46", "ARM #47", "ARM #48", "ARM #49",
	"ARM #50", "ARM #51", "ARM #52", "ARM #53", "ARM #54",
	"ARM #55", "ARM #56", "ARM #57", "ARM #58", "ARM #59",
	"ARM #60", "ARM #61", "ARM #62", "ARM #63", "ARM #64",
	"ARM #65", "ARM #66", "ARM #67", "ARM #68", "ARM #69",
	"ARM #70", "ARM #71", "ARM #72", "ARM #73", "ARM #74",
	"ARM #75", "ARM #76", "ARM #77", "ARM #78", "ARM #79",
	"ARM #80", "ARM #81", "ARM #82", "ARM #83", "ARM #84",
	"ARM #85", "ARM #86", "ARM #87", "ARM #88", "ARM #89",
	"ARM #90", "ARM #91", "ARM #92", "ARM #93", "ARM #94",
	"ARM #95", "ARM #96", "ARM #97", "ARM #98", "ARM #99",
	"ARM_GNU_VTENTRY",
	"ARM_GNU_VTINHERIT",
	"ARM_THM_PC11",
	"ARM_THM_PC9",
	"ARM #104",
	"ARM #105",
	"ARM #106",
	"ARM #107",
	"ARM #108",
	"ARM #109",
	"ARM #110", "ARM #111", "ARM #112", "ARM #113", "ARM #114",
	"ARM #115", "ARM #116", "ARM #117", "ARM #118", "ARM #119",
	"ARM #120", "ARM #121", "ARM #122", "ARM #123", "ARM #124",
	"ARM #125", "ARM #126", "ARM #127", "ARM #128", "ARM #129",
	"ARM #130", "ARM #131", "ARM #132", "ARM #133", "ARM #134",
	"ARM #135", "ARM #136", "ARM #137", "ARM #138", "ARM #139",
	"ARM #140", "ARM #141", "ARM #142", "ARM #143", "ARM #144",
	"ARM #145", "ARM #146", "ARM #147", "ARM #148", "ARM #149",
	"ARM #150", "ARM #151", "ARM #152", "ARM #153", "ARM #154",
	"ARM #155", "ARM #156", "ARM #157", "ARM #158", "ARM #159",
	"ARM #160", "ARM #161", "ARM #162", "ARM #163", "ARM #164",
	"ARM #165", "ARM #166", "ARM #167", "ARM #168", "ARM #169",
	"ARM #170", "ARM #171", "ARM #172", "ARM #173", "ARM #174",
	"ARM #175", "ARM #176", "ARM #177", "ARM #178", "ARM #179",
	"ARM #180", "ARM #181", "ARM #182", "ARM #183", "ARM #184",
	"ARM #185", "ARM #186", "ARM #187", "ARM #188", "ARM #189",
	"ARM #190", "ARM #191", "ARM #192", "ARM #193", "ARM #194",
	"ARM #195", "ARM #196", "ARM #197", "ARM #198", "ARM #199",
	"ARM #200", "ARM #201", "ARM #202", "ARM #203", "ARM #204",
	"ARM #205", "ARM #206", "ARM #207", "ARM #208", "ARM #209",
	"ARM #210", "ARM #211", "ARM #212", "ARM #213", "ARM #214",
	"ARM #215", "ARM #216", "ARM #217", "ARM #218", "ARM #219",
	"ARM #220", "ARM #221", "ARM #222", "ARM #223", "ARM #224",
	"ARM #225", "ARM #226", "ARM #227", "ARM #228", "ARM #229",
	"ARM #230", "ARM #231", "ARM #232", "ARM #233", "ARM #234",
	"ARM #235", "ARM #236", "ARM #237", "ARM #238", "ARM #239",
	"ARM #240", "ARM #241", "ARM #242", "ARM #243", "ARM #244",
	"ARM #245", "ARM #246", "ARM #247", "ARM #248",
	"ARM_RXPC25",
	"ARM_RSBREL32",
	"ARM_THM_RPC22",
	"ARM_RREL32",
	"ARM_RABS32",
	"ARM_RPC24",
	"ARM_RBASE",
};

const char *const elf_rel386[] = {
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
	"I386 #11",
	"I386 #12",
	"I386 #13",
	"I386 #14",
	"I386 #15",
	"I386 #16",
	"I386 #17",
	"I386 #18",
	"I386 #19",
	"I386_16",
	"I386_PC16",
	"I386_8",
	"I386_PC8",
};

const char *const elf_rel68k[] = {
};

const char *const elf_rel88k[] = {
};

const char *const elf_relppc[] = {
	"NONE",
	"PPC_32",
	"PPC_24",
	"PPC_16",
	"PPC_16_LO",
	"PPC_16_HI",
	"PPC_16_HA",
	"PPC_14",
	"PPC_14_TAKEN",
	"PPC_14_NTAKEN",
	"PPC_REL24",
	"PPC_REL14",
	"PPC_REL14_TAKEN",
	"PPC_REL14_NTAKEN",
	"PPC_GOT16",
	"PPC_GOT16_LO",
	"PPC_GOT16_HI",
	"PPC_GOT16_HA",
	"PPC_PLT24",
	"PPC_COPY",
	"PPC_DAT",
	"PPC_JMP_SLOT",
	"PPC_RELATIVE",
	"PPC_LOCAL24PC",
	"PPC_U32",
	"PPC_U16",
	"PPC_REL32",
	"PPC_PLT32",
	"PPC_PLTREL32",
	"PPC_PLT16_LO",
	"PPC_PLT16_HI",
	"PPC_PLT16_HA",
	"PPC_SDAREL",
};

const char *const elf_relmips[] = {
	"NONE",
	"MIPS_16",
	"MIPS_32",
	"MIPS_REL32",
	"MIPS_26",
	"MIPS_HI16",
	"MIPS_LO16",
	"MIPS_GPREL16",
	"MIPS_LITERAL",
	"MIPS_GOT16",
	"MIPS_PC16",
	"MIPS_CALL16",
	"MIPS_GPREL32",
	"MIPS_64",
};

const char *const elf_relparisc[] = {
	"NONE",
	"PARISC_DIR32",
	"PARISC_DIR21L",
	"PARISC_DIR17R",
	"PARISC_DIR17F",
	"PARISC #5",
	"PARISC_DIR14R",
	"PARISC #7",
	"PARISC_PCREL12F",
	"PARISC_PCREL32",
	"PARISC_PCREL21L",
	"PARISC_PCREL17R",
	"PARISC_PCREL17F",
	"PARISC_PCREL17C",
	"PARISC_PCREL14R",
	"PARISC #15",
	"PARISC #16",
	"PARISC #17",
	"PARISC_DPREL21L",
	"PARISC_DPREL14WR",
	"PARISC_DPREL14DR",
	"PARISC #21",
	"PARISC_DPREL14R",
	"PARISC #23",
	"PARISC #24",
	"PARISC #25",
	"PARISC_GPREL21L",
	"PARISC #27",
	"PARISC #28",
	"PARISC #29",
	"PARISC_GPREL14R",
	"PARISC #31",
	"PARISC #32",
	"PARISC #33",
	"PARISC_LTOFF21L",
	"PARISC #35",
	"PARISC #36",
	"PARISC #37",
	"PARISC_LTOFF14R",
	"PARISC_LTOFF14F",
	"PARISC_SETBASE",
	"PARISC_SECREL32",
	"PARISC_BASEREL21L",
	"PARISC_BASEREL17R",
	"PARISC #44",
	"PARISC #45",
	"PARISC_BASEREL14R",
	"PARISC #47",
	"PARISC_SEGBASE",
	"PARISC_SEGREL32",
	"PARISC_PLTOFF21L",
	"PARISC #51",
	"PARISC #52",
	"PARISC #53",
	"PARISC_PLTOFF14R",
	"PARISC_PLTOFF14F",
	"PARISC #56",
	"PARISC_LTOFF_FPTR32",
	"PARISC_LTOFF_FPTR21L",
	"PARISC #59",
	"PARISC #60",
	"PARISC #61",
	"PARISC_LTOFF_FPTR14R",
	"PARISC #63",
	"PARISC_FPTR64",
	"PARISC_PLABEL32",
	"PARISC #66",
	"PARISC #67",
	"PARISC #68",
	"PARISC #69",
	"PARISC #70",
	"PARISC #71",
	"PARISC_PCREL64",
	"PARISC_PCREL22C",
	"PARISC_PCREL22F",
	"PARISC_PCREL14WR",
	"PARISC_PCREL14DR",
	"PARISC_PCREL16F",
	"PARISC_PCREL16WF",
	"PARISC_PCREL16DF",
	"PARISC_DIR64",
	"PARISC #81",
	"PARISC #82",
	"PARISC_DIR14WR",
	"PARISC_DIR14DR",
	"PARISC_DIR16F",
	"PARISC_DIR16WF",
	"PARISC_DIR16DF",
	"PARISC_GPREL64",
	"PARISC #89",
	"PARISC #90",
	"PARISC_GPREL14WR",
	"PARISC_GPREL14DR",
	"PARISC_GPREL16F",
	"PARISC_GPREL16WF",
	"PARISC_GPREL16DF",
	"PARISC_LTOFF64",
	"PARISC #97",
	"PARISC #98",
	"PARISC_LTOFF14WR",
	"PARISC_LTOFF14DR",
	"PARISC_LTOFF16F",
	"PARISC_LTOFF16WF",
	"PARISC_LTOFF16DF",
	"PARISC_SECREL64",
	"PARISC #105",
	"PARISC #106",
	"PARISC_BASEREL14WR",
	"PARISC_BASEREL14DR",
	"PARISC #109",
	"PARISC #110",
	"PARISC #111",
	"PARISC_SEGREL64",
	"PARISC #113",
	"PARISC #114",
	"PARISC_PLTOFF14WR",
	"PARISC_PLTOFF14DR",
	"PARISC_PLTOFF16F",
	"PARISC_PLTOFF16WF",
	"PARISC_PLTOFF16DF",
	"PARISC_LTOFF_FPTR64",
	"PARISC #121",
	"PARISC #122",
	"PARISC_LTOFF_FPTR14WR",
	"PARISC_LTOFF_FPTR14DR",
	"PARISC_LTOFF_FPTR16F",
	"PARISC_LTOFF_FPTR16WF",
	"PARISC_LTOFF_FPTR16DF",
	"PARISC_COPY",
	"PARISC_IPLT",
	"PARISC_EPLT",
	"PARISC_GDATA",
	"PARISC_JMPSLOT",
	"PARISC_RELATIVE",
};

const char *const elf_relsh[] = {
	"NONE",
#define	R_SH_DIR32				1
#define	R_SH_REL32				2
#define	R_SH_DIR8WPN				3
#define	R_SH_IND12W				4
#define	R_SH_DIR8WPL				5
#define	R_SH_DIR8WPZ				6
#define	R_SH_DIR8BP				7
#define	R_SH_DIR8W				8
#define	R_SH_DIR8L				9

/* GNU extensions */
#define	R_SH_LOOP_START				10
#define	R_SH_LOOP_END				11
#define	R_SH_GNU_VTINHERIT			22
#define	R_SH_GNU_VTENTRY			23
#define	R_SH_SWITCH8				24
#define	R_SH_SWITCH16				25
#define	R_SH_SWITCH32				26
#define	R_SH_USES				27
#define	R_SH_COUNT				28
#define	R_SH_ALIGN				29
#define	R_SH_CODE				30
#define	R_SH_DATA				31
#define	R_SH_LABEL				32

#define	R_SH_DIR16				33
#define	R_SH_DIR8				34
#define	R_SH_DIR8UL				35
#define	R_SH_DIR8UW				36
#define	R_SH_DIR8U				37
#define	R_SH_DIR8SW				38
#define	R_SH_DIR8S				39
#define	R_SH_DIR4UL				40
#define	R_SH_DIR4UW				41
#define	R_SH_DIR4U				42
#define	R_SH_PSHA				43
#define	R_SH_PSHL				44
#define	R_SH_DIR5U				45
#define	R_SH_DIR6U				46
#define	R_SH_DIR6S				47
#define	R_SH_DIR10S				48
#define	R_SH_DIR10SW				49
#define	R_SH_DIR10SL				50
#define	R_SH_DIR10SQ				51
#define	R_SH_DIR16S				53

/* GNU extensions */
#define	R_SH_TLS_GD_32				144
#define	R_SH_TLS_LD_32				145
#define	R_SH_TLS_LDO_32				146
#define	R_SH_TLS_IE_32				147
#define	R_SH_TLS_LE_32				148
#define	R_SH_TLS_DTPMOD32			149
#define	R_SH_TLS_DTPOFF32			150
#define	R_SH_TLS_TPOFF32			151
#define	R_SH_GOT32				160
#define	R_SH_PLT32				161
#define	R_SH_COPY				162
#define	R_SH_GLOB_DAT				163
#define	R_SH_JMP_SLOT				164
#define	R_SH_RELATIVE				165
#define	R_SH_GOTOFF				166
#define	R_SH_GOTPC				167
#define	R_SH_GOTPLT32				168
#define	R_SH_GOT_LOW16				169
#define	R_SH_GOT_MEDLOW16			170
#define	R_SH_GOT_MEDHI16			171
#define	R_SH_GOT_HI16				172
#define	R_SH_GOTPLT_LOW16			173
#define	R_SH_GOTPLT_MEDLOW16			174
#define	R_SH_GOTPLT_MEDHI16			175
#define	R_SH_GOTPLT_HI16			176
#define	R_SH_PLT_LOW16				177
#define	R_SH_PLT_MEDLOW16			178
#define	R_SH_PLT_MEDHI16			179
#define	R_SH_PLT_HI16				180
#define	R_SH_GOTOFF_LOW16			181
#define	R_SH_GOTOFF_MEDLOW16			182
#define	R_SH_GOTOFF_MEDHI16			183
#define	R_SH_GOTOFF_HI16			184
#define	R_SH_GOTPC_LOW16			185
#define	R_SH_GOTPC_MEDLOW16			186
#define	R_SH_GOTPC_MEDHI16			187
#define	R_SH_GOTPC_HI16				188
#define	R_SH_GOT10BY4				189
#define	R_SH_GOTPLT10BY4			190
#define	R_SH_GOT10BY8				191
#define	R_SH_GOTPLT10BY8			192
#define	R_SH_COPY64				193
#define	R_SH_GLOB_DAT64				194
#define	R_SH_JMP_SLOT64				195
#define	R_SH_RELATIVE64				196
#define	R_SH_SHMEDIA_CODE			242
#define	R_SH_PT_16				243
#define	R_SH_IMMS16				244
#define	R_SH_IMMU16				245
#define	R_SH_IMM_LOW16				246
#define	R_SH_IMM_LOW16_PCREL			247
#define	R_SH_IMM_MEDLOW16			248
#define	R_SH_IMM_MEDLOW16_PCREL			249
#define	R_SH_IMM_MEDHI16			250
#define	R_SH_IMM_MEDHI16_PCREL			251
#define	R_SH_IMM_HI16				252
#define	R_SH_IMM_HI16_PCREL			253
#define	R_SH_64					254
#define	R_SH_64_PCREL				255
};

const char *const elf_relsparc[] = {
	"NONE",
	"SPARC #1", "SPARC #2", "SPARC #3", "SPARC #4",
	"SPARC #5", "SPARC #6", "SPARC #7", "SPARC #8", "SPARC #9",
	"SPARC #10", "SPARC #11", "SPARC #12", "SPARC #13", "SPARC #14",
	"SPARC #15", "SPARC #16", "SPARC #17", "SPARC #18",
	"SPARC_COPY",
	"SPARC_GLOB_DAT",
	"SPARC_JMP_SLOT",
	"SPARC_RELATIVE",
	"SPARC #23", "SPARC #24",
	"SPARC #25", "SPARC #26", "SPARC #27", "SPARC #28", "SPARC #29",
	"SPARC #30", "SPARC #31",
	"SPARC_64",
	"SPARC #33", "SPARC #34",
	"SPARC #35", "SPARC #36", "SPARC #37", "SPARC #38", "SPARC #39",
	"SPARC #40", "SPARC #41", "SPARC #42", "SPARC #43", "SPARC #44",
	"SPARC #45", "SPARC #46", "SPARC #47", "SPARC #48", "SPARC #49",
	"SPARC #50", "SPARC #51", "SPARC #52", "SPARC #53",
	"SPARC_UA64",
};

const char *const elf_relvax[] = {
};

const char *const elf_relia64[] = {
};

const char *
elf_reltype(int m, int r)
{
	static char buf[32];
	const char *const*b;
	int n;

	switch (m) {
	case EM_ALPHA:
		b = elf_relalpha;
		n = sizeof elf_relalpha / sizeof elf_relalpha[0];
		break;
	case EM_AMD64:
		b = elf_relamd64;
		n = sizeof elf_relamd64 / sizeof elf_relamd64[0];
		break;
	case EM_ARM:
		b = elf_relarm;
		n = sizeof elf_relarm / sizeof elf_relarm[0];
		break;
	case EM_386:
		b = elf_rel386;
		n = sizeof elf_rel386 / sizeof elf_rel386[0];
		break;
	case EM_68K:
		b = elf_rel68k;
		n = sizeof elf_rel68k / sizeof elf_rel68k[0];
		break;
	case EM_88K:
		b = elf_rel88k;
		n = sizeof elf_rel88k / sizeof elf_rel88k[0];
		break;
	case EM_PPC:
		b = elf_relppc;
		n = sizeof elf_relppc / sizeof elf_relppc[0];
		break;
	case EM_MIPS:
		b = elf_relmips;
		n = sizeof elf_relmips / sizeof elf_relmips[0];
		break;
	case EM_PARISC:
		b = elf_relparisc;
		n = sizeof elf_relparisc / sizeof elf_relparisc[0];
		break;
	case EM_SH:
		b = elf_relsh;
		n = sizeof elf_relsh / sizeof elf_relsh[0];
		break;
	case EM_SPARC:
	case EM_SPARC64:
		b = elf_relsparc;
		n = sizeof elf_relsparc / sizeof elf_relsparc[0];
		break;
	case EM_VAX:
		b = elf_relvax;
		n = sizeof elf_relvax / sizeof elf_relvax[0];
	default:
		snprintf(buf, sizeof buf, "0x%x", r);
		return buf;
	}

	if (r >= 0 && r < n)
		return b[r];

	snprintf(buf, sizeof buf, "0x%x", r);
	return buf;
}
