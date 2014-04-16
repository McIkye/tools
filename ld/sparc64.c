/*
 * Copyright (c) 2013-2014 Michael Shalayeff
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
    "$ABSD: sparc64.c,v 1.2 2013/10/29 12:07:05 mickey Exp $";
#endif

#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <elf_abi.h>
#include <dwarf.h>
#include <elfuncs.h>
#include <a.out.h>
#include <err.h>

#include <sparc64/exec.h>

#include "ld.h"

/* fill text with smth illigal to avoid nop slides */
#define	XFILL	0xcecececececececeULL	/* into */
#define	DFILL	0xd0d0d0d0d0d0d0d0ULL
const struct ldorder sparc64_order[] = {
	{ ldo_symbol,	"_start", N_UNDF, 0, LD_ENTRY },
	{ ldo_interp,	ELF_INTERP, SHT_PROGBITS, SHF_ALLOC,
			LD_CONTAINS | LD_DYNAMIC | LD_USED },
	{ ldo_section,	ELF_NOTE, SHT_NOTE, SHF_ALLOC,
			LD_NONMAGIC | LD_NOOMAGIC | LD_USED },
	{ ldo_section,	ELF_INIT, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR,
			LD_USED, XFILL },
	{ ldo_section,	ELF_PLT, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR,
			LD_DYNAMIC, XFILL },
	{ ldo_section,	ELF_TEXT, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR,
			0, XFILL },
	{ ldo_section,	ELF_GCC_LINK1T, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR,
			LD_LINK1, XFILL },
	{ ldo_section,	ELF_FINI, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR,
			LD_USED, XFILL },
	{ ldo_expr,	". += 0x1000", 0, 0, LD_NOOMAGIC },
	{ ldo_symbol,	"etext", N_ABS },
	{ ldo_symbol,	"_etext", N_ABS },
	{ ldo_expr,	". += 0x1000", 0, 0, LD_NOOMAGIC },
	{ ldo_section,	ELF_RODATA, SHT_PROGBITS, SHF_ALLOC,
			0, DFILL },
	{ ldo_section,	ELF_GCC_LINK1R, SHT_PROGBITS, SHF_ALLOC, LD_LINK1 },
	{ ldo_ehfrh,	ELF_EH_FRAME_H, SHT_PROGBITS, SHF_ALLOC,
			LD_CONTAINS | LD_IGNORE },
	{ ldo_section,	ELF_EH_FRAME, SHT_PROGBITS, SHF_ALLOC, LD_IGNORE },
	{ ldo_section,	ELF_GCC_EXCEPT, SHT_PROGBITS, SHF_ALLOC, LD_IGNORE },
	{ ldo_expr,	". += 0x1000", 0, 0, LD_NOOMAGIC },
	{ ldo_symbol,	"__data_start", N_ABS },
	{ ldo_section,	ELF_SDATA, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE },
	{ ldo_section,	ELF_DATA, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE,
			0, DFILL },
	{ ldo_section,	ELF_GCC_LINK1D, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE,
			LD_LINK1 },
	{ ldo_section,	ELF_CTORS, SHT_PROGBITS, SHF_ALLOC|SHF_WRITE, LD_USED },
	{ ldo_section,	ELF_DTORS, SHT_PROGBITS, SHF_ALLOC|SHF_WRITE, LD_USED },
	{ ldo_expr,	". += 0x1000", 0, 0, LD_NOOMAGIC },
	{ ldo_symbol,	"__got_start", N_ABS, 0, LD_DYNAMIC },
	{ ldo_section,	ELF_GOT, SHT_PROGBITS, SHF_ALLOC, LD_DYNAMIC },
	{ ldo_symbol,	"__got_end", N_ABS, 0, LD_DYNAMIC },
	{ ldo_symbol,	"edata", N_ABS },
	{ ldo_symbol,	"_edata", N_ABS },
	{ ldo_expr,	". += 0x1000", 0, 0, LD_NOOMAGIC },
	{ ldo_symbol,	"__bss_start", N_ABS },
	{ ldo_section,	ELF_SBSS, SHT_NOBITS, SHF_ALLOC | SHF_WRITE },
	{ ldo_section,	ELF_BSS, SHT_NOBITS, SHF_ALLOC | SHF_WRITE },
	{ ldo_section,	ELF_GCC_LINK1B, SHT_NOBITS, SHF_ALLOC | SHF_WRITE,
			LD_LINK1 },
	{ ldo_symbol,	"end", N_ABS },
	{ ldo_symbol,	"_end", N_ABS },
	{ ldo_shstr,	ELF_SHSTRTAB, SHT_STRTAB, 0, LD_CONTAINS },

	  /* section headers go here */

	{ ldo_symtab,	ELF_SYMTAB, SHT_SYMTAB, 0, LD_CONTAINS | LD_SYMTAB },
	{ ldo_strtab,	ELF_STRTAB, SHT_STRTAB, 0, LD_CONTAINS | LD_SYMTAB },

	  /* stabs debugging sections */
	{ ldo_section,	ELF_STAB, SHT_PROGBITS, 0, LD_DEBUG },
	{ ldo_section,	ELF_STABSTR, SHT_PROGBITS, 0, LD_DEBUG },
	{ ldo_section,	ELF_STAB_EXCL, SHT_PROGBITS, 0, LD_DEBUG },
	{ ldo_section,	ELF_STAB_EXCLS, SHT_PROGBITS, 0, LD_DEBUG },
	{ ldo_section,	ELF_STAB_INDEX, SHT_PROGBITS, 0, LD_DEBUG },
	{ ldo_section,	ELF_STAB_IDXSTR, SHT_PROGBITS, 0, LD_DEBUG },
/*	{ ldo_section,	ELF_STAB_COMM, SHT_PROGBITS, 0, LD_DEBUG }, */
#if 0
	  /* dwarf mark II debugging sections */
	{ ldo_section,	DWARF_ABBREV, SHT_PROGBITS, 0, LD_DEBUG },
	{ ldo_section,	DWARF_ARANGES, SHT_PROGBITS, 0, LD_DEBUG },
	{ ldo_section,	DWARF_FRAMES, SHT_PROGBITS, 0, LD_DEBUG },
	{ ldo_section,	DWARF_INFO, SHT_PROGBITS, 0, LD_DEBUG },
	{ ldo_section,	DWARF_LINES, SHT_PROGBITS, 0, LD_DEBUG },
	{ ldo_section,	DWARF_LOC, SHT_PROGBITS, 0, LD_DEBUG },
	{ ldo_section,	DWARF_MACROS, SHT_PROGBITS, 0, LD_DEBUG },
	{ ldo_section,	DWARF_PUBNAMES, SHT_PROGBITS, 0, LD_DEBUG },
	{ ldo_section,	DWARF_STR, SHT_PROGBITS, 0, LD_DEBUG },
#endif
	{ ldo_kaput }
};

int
sparc64_fix(off_t off, struct section *os, char *sbuf, int len)
{
	struct relist *rp = os->os_rels, *erp = rp + os->os_nrls;
	Elf64_Shdr *shdr = os->os_sect;
	char *p, *ep;
	uint64_t a64;
	uint32_t a32;
	int reoff;

/* this evil little loop has to be optimised; for example store last rp on os */
	for (; rp < erp; rp++)
		if (off <= rp->rl_addr)
			break;

	if (rp == erp)
		return 0;

	for (p = sbuf + rp->rl_addr - off, ep = sbuf + len;
	    p < ep; p += reoff, rp++) {

		if (rp >= erp)
			errx(1, "sparc64_fix: botch1");

		if (rp + 1 < erp)
			reoff = rp[1].rl_addr - rp[0].rl_addr;
		else
			reoff = ep - p;

		switch (rp->rl_type) {
		case R_SPARC_UA64:
		case R_SPARC_64:
			if (ep - p < 8)
				return ep - p;
			if (rp->rl_sym->sl_name)
				a64 = rp->rl_sym->sl_elfsym.sym64.st_value;
			else
				a64 = ((Elf64_Shdr *)
				    rp->rl_sym->sl_sect->os_sect)->sh_addr;
			sparc64_fixone(p, a64, rp->rl_addend, rp->rl_type);
			break;

		case R_SPARC_UA32:
		case R_SPARC_32:

		case R_SPARC_WDISP30:

		case R_SPARC_H44:
		case R_SPARC_M44:
		case R_SPARC_L44:
		case R_SPARC_HI22:
		case R_SPARC_LO10:
			if (ep - p < 4)
				return ep - p;
			if (rp->rl_sym->sl_name)
				a32 = rp->rl_sym->sl_elfsym.sym64.st_value;
			else
				a32 = ((Elf64_Shdr *)
				    rp->rl_sym->sl_sect->os_sect)->sh_addr;
			if (rp->rl_type == R_SPARC_WDISP30)
				a32 -= shdr->sh_addr + rp->rl_addr;

			sparc64_fixone(p, a32, rp->rl_addend, rp->rl_type);
			break;

		case R_SPARC_COPY:
		case R_SPARC_GLOB_DAT:
		case R_SPARC_JMP_SLOT:
		case R_SPARC_RELATIVE:

		case R_SPARC_NONE:
		default:
			errx(1, "unknown reloc type %d", rp->rl_type);
		}
	}

	return 0;
}

int
sparc64_fixone(char *p, uint64_t val, int64_t addend, uint type)
{
	uint64_t v64;
	uint32_t v32;

	/* seems that only R_SPARC_UA* relocs 'll be unaligned,
	 * but for now we're doing the copy out on every until
	 * we have all things fixed */
	switch (type) {
	case R_SPARC_UA64:

	case R_SPARC_64:
		memcpy(&v64, p, sizeof v64);
		v64 = betoh64(v64);
		v64 += val + addend;
		v64 = htobe64(v64);
		memcpy(p, &v64, sizeof v64);
		break;

	case R_SPARC_UA32:
	case R_SPARC_32:
	case R_SPARC_WDISP30:
	case R_SPARC_H44:
	case R_SPARC_M44:
	case R_SPARC_L44:
	case R_SPARC_HI22:
	case R_SPARC_LO10:
		memcpy(&v32, p, sizeof v32);
		v32 = betoh32(v32);
		switch (type) {
		case R_SPARC_UA32:

		case R_SPARC_32:
			v32 += val + addend;
			break;
		case R_SPARC_WDISP30:
			v32 &= ~0x3fffffff;
			v32 |= (val >> 2) & 0x3fffffff;
			break;
		case R_SPARC_H44:
			v32 &= ~0x3fffff;
			v32 |= (val >> 22) & 0x3fffff;
			break;
		case R_SPARC_M44:
			v32 &= ~0x3ff;
			v32 |= (val >> 12) & 0x3ff;
			break;
		case R_SPARC_L44:
			v32 &= ~0xfff;
			v32 |= val & 0xfff;
			break;
		case R_SPARC_HI22:
			v32 &= ~0x3fffff;
			v32 |= (val >> 10) & 0x3fffff;
			break;
		case R_SPARC_LO10:
			v32 &= ~0x3ff;
			v32 |= val & 0x3ff;	
		}
		v32 = htobe32(v32);
		memcpy(p, &v32, sizeof v32);
		break;
	default:
		errx(1, "unknown reloc type %d", type);
	}

	return 0;
}
