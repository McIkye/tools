/*
 * Copyright (c) 2011-2014 Michael Shalayeff
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
static const char rcsid[] = "$ABSD: hppa.c,v 1.5 2013/10/29 12:07:05 mickey Exp $";
#endif

#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <elf_abi.h>
#include <dwarf.h>
#include <elfuncs.h>
#include <a.out.h>
#include <err.h>

#include <hppa/reloc.h>

#include "ld.h"

#define	ELF_PA_UNWIND	".PARISC.unwind"
#define	ELF_PA_UNWINFO	".PARISC.unwind_info"

/* fill text with smth illigal to avoid nop slides */
#define	XFILL	0x03ffe01f03ffe01fULL	/* break 0x1f, 0x1fff */
#define	DFILL	0xd0d0d0d0d0d0d0d0ULL
const struct ldorder hppa_order[] = {
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
	{ ldo_section,	ELF_RODATA, SHT_PROGBITS, SHF_ALLOC, 0, DFILL },
	{ ldo_section,	ELF_GCC_LINK1R, SHT_PROGBITS, SHF_ALLOC, LD_LINK1,
			DFILL },
	{ ldo_ehfrh,	ELF_EH_FRAME_H, SHT_PROGBITS, SHF_ALLOC,
			LD_CONTAINS | LD_IGNORE },
	{ ldo_section,	ELF_EH_FRAME, SHT_PROGBITS, SHF_ALLOC, LD_IGNORE },
	{ ldo_section,	ELF_GCC_EXCEPT, SHT_PROGBITS, SHF_ALLOC, LD_IGNORE },
	{ ldo_section,	ELF_PA_UNWINFO, SHT_PROGBITS, SHF_ALLOC, LD_IGNORE },
	{ ldo_section,	ELF_PA_UNWIND, SHT_PROGBITS, SHF_ALLOC, LD_IGNORE },
	{ ldo_expr,	". += 0x1000", 0, 0, LD_NOOMAGIC },
	{ ldo_symbol,	"__data_start", N_ABS },
	{ ldo_section,	ELF_SDATA, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE },
	{ ldo_section,	ELF_DATA, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE,
			0, DFILL },
	{ ldo_section,	ELF_GCC_LINK1D, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE,
			LD_LINK1, DFILL },
	{ ldo_section,	ELF_CTORS, SHT_PROGBITS, SHF_ALLOC|SHF_WRITE, LD_USED },
	{ ldo_section,	ELF_DTORS, SHT_PROGBITS, SHF_ALLOC|SHF_WRITE, LD_USED },
	{ ldo_expr,	". += 0x1000", 0, 0, LD_NOOMAGIC },
	{ ldo_symbol,	"__got_start", N_ABS, 0, LD_DYNAMIC },
	{ ldo_symbol,	"_GLOBAL_OFFSET_TABLE_", N_ABS },
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
hppa_fix(off_t off, struct section *os, char *sbuf, int len)
{
	static uint64_t sb;	/* segment base */
	struct relist *rp = os->os_rels, *erp = rp + os->os_nrls;
	Elf32_Shdr *shdr = os->os_sect;
	char *p, *ep;
	uint64_t a64;
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
			errx(1, "i386_fix: botch1");

		if (rp + 1 < erp)
			reoff = rp[1].rl_addr - rp[0].rl_addr;
		else
			reoff = ep - p;

		if (ep - p < 4)
			return ep - p;

		if (rp->rl_sym->sl_name)
			a64 = rp->rl_sym->sl_elfsym.sym32.st_value;
		else
			a64 = ((Elf32_Shdr *)
			    rp->rl_sym->sl_sect->os_sect)->sh_addr;

		switch (rp->rl_type) {
		case RELOC_NONE:
			break;

		case RELOC_DIR32:
		case RELOC_DIR64:
		case RELOC_DIR21L:
		case RELOC_DIR17R:
		case RELOC_DIR17F:
		case RELOC_DIR14R:
			hppa_fixone(p, a64, rp->rl_addend, rp->rl_type);
			break;

		case RELOC_PCREL32:
		case RELOC_PCREL64:
		case RELOC_PCREL22C:
		case RELOC_PCREL22F:
		case RELOC_PCREL21L:
		case RELOC_PCREL17R:
		case RELOC_PCREL17F:
		case RELOC_PCREL17C:
		case RELOC_PCREL16F:
		case RELOC_PCREL16WF:
		case RELOC_PCREL16DF:
		case RELOC_PCREL14R:
		case RELOC_PCREL14WR:
		case RELOC_PCREL14DR:
		case RELOC_PCREL12F:
			a64 -= shdr->sh_addr + rp->rl_addr + 8;
			hppa_fixone(p, a64, rp->rl_addend, rp->rl_type);
			break;

		case RELOC_DPREL21L:
		case RELOC_DPREL14WR:
		case RELOC_DPREL14DR:
		case RELOC_DPREL14R:
		case RELOC_GPREL21L:
		case RELOC_GPREL14R:
			a64 -= shdr->sh_addr /* + GP */;
			hppa_fixone(p, a64, rp->rl_addend, rp->rl_type);
			break;

		case RELOC_LTOFF21L:
		case RELOC_LTOFF14R:
		case RELOC_LTOFF14F:
			break;

		case RELOC_PLABEL32:
			hppa_fixone(p, a64, rp->rl_addend, rp->rl_type);
			break;

		case RELOC_SECREL32:
			a64 -= shdr->sh_addr;
			hppa_fixone(p, a64, rp->rl_addend, rp->rl_type);
			break;

		case RELOC_SEGBASE:
			sb = a64;
			break;

		case RELOC_SEGREL32:
			a64 -= sb;
			hppa_fixone(p, a64, rp->rl_addend, rp->rl_type);
			break;

		case RELOC_SETBASE:
		case RELOC_BASEREL21L:
		case RELOC_BASEREL17R:
		case RELOC_BASEREL14R:
		case RELOC_PLTOFF21L:
		case RELOC_PLTOFF14R:
		case RELOC_PLTOFF14F:
		case RELOC_LTOFF_FPTR32:
		case RELOC_LTOFF_FPTR21L:
		case RELOC_LTOFF_FPTR14R:
		case RELOC_FPTR64:
		case RELOC_DIR14WR:
		case RELOC_DIR14DR:
		case RELOC_DIR16F:
		case RELOC_DIR16WF:
		case RELOC_DIR16DF:
		case RELOC_GPREL64:
		case RELOC_GPREL14WR:
		case RELOC_GPREL14DR:
		case RELOC_GPREL16F:
		case RELOC_GPREL16WF:
		case RELOC_GPREL16DF:
		case RELOC_LTOFF64:
		case RELOC_LTOFF14WR:
		case RELOC_LTOFF14DR:
		case RELOC_LTOFF16F:
		case RELOC_LTOFF16WF:
		case RELOC_LTOFF16DF:
		case RELOC_SECREL64:
		case RELOC_BASEREL14WR:
		case RELOC_BASEREL14DR:
		case RELOC_SEGREL64:
		case RELOC_PLTOFF14WR:
		case RELOC_PLTOFF14DR:
		case RELOC_PLTOFF16F:
		case RELOC_PLTOFF16WF:
		case RELOC_PLTOFF16DF:
		case RELOC_LTOFF_FPTR64:
		case RELOC_LTOFF_FPTR14WR:
		case RELOC_LTOFF_FPTR14DR:
		case RELOC_LTOFF_FPTR16F:
		case RELOC_LTOFF_FPTR16WF:
		case RELOC_LTOFF_FPTR16DF:
		case RELOC_COPY:
		case RELOC_IPLT:
		case RELOC_EPLT:
		case RELOC_GDATA:
		case RELOC_JMPSLOT:
		case RELOC_RELATIVE:
		default:
			errx(1, "%s: unknown reloc type %d",
			    os->os_obj->ol_name, rp->rl_type);
		}
	}

	return 0;
}

int
hppa_fixone(char *p, uint64_t val, int64_t addend, uint type)
{
	uint32_t v32;

	memcpy(&v32, p, sizeof v32);
	v32 = betoh32(v32);
	switch (type) {
	case RELOC_DIR32:
	case RELOC_PCREL32:
	case RELOC_SECREL32:
	case RELOC_SEGREL32:
		v32 += val + addend;
		break;

#define	RND(v)		(((v) + 0x1000) & ~0x1fff)

#define	IMM22(v)	((((v) & 0x3ff0000) >> 3) | (((v) & 4) << 10) | \
	(((v) & 0x1ff8) >> 1) | ((v) & 1? ~0x8fffff : 0)) 
#define	ASS22(v)	((((v) >> 21) & 1) | (((v) & 0x7fe000) << 3) | \
	(((v) >> 10) & 4) | (((v) & 0xffc) << 1))

	case RELOC_PCREL22C:
	case RELOC_PCREL22F:
		val += IMM22(v32) + addend;
		v32 &= 0xfc00e002;
		v32 |= ASS22(val);
		break;

#define	IMM21(v)	((((v) & 0xffe) << 30) | (((v) & 0xc000) << 4) | \
	(((v) & 0x1f0000) >> 3) | (((v) & 0x3000) >> 1) | ((v) & 1? ~0xfffff:0))
#define	ASS21(v)	((((v) >> 20) & 1) | (((v) & 0x7ff00000) >> 9) | \
	(((v) & 0xc0000) >> 4) | (((v) & 0x37000) << 3) | (((v) & 1800) << 1))

	case RELOC_PCREL21L:
		val += IMM21(v32) + addend;
		v32 &= 0xffe00000;
		v32 |= ASS21(val);
		break;

	case RELOC_DIR21L:
	case RELOC_DPREL21L:
	case RELOC_GPREL21L:
		val += IMM21(v32) + RND(addend);
		v32 &= 0xffe00000;
		v32 |= ASS21(val);
		break;

#define	IMM17(v)	((((v) & 0x3e000) >> 3) | (((v) & 4) << 10) | \
	(((v) & 0x1ff8) >> 1) | ((v) & 1? ~0x80000 : 0))
#define	ASS17(v)	((((v) & 0x7c00) << 3) | (((v) & 0x1000) >> 10) | \
	(((v) & 0xffc) << 1) | (((v) & 0x10000) >> 16))

	case RELOC_DIR17R:
		val += IMM17(v32) + RND(addend);
		v32 &= 0xffe0e002;
		v32 |= ASS17(val);
		break;

	case RELOC_PCREL17R:
		val += IMM17(v32) + addend;
		val &= 0x7ff;
		v32 &= 0xffe0e002;
		v32 |= ASS17(val);
		break;

	case RELOC_DIR17F:
	case RELOC_PCREL17F:
	case RELOC_PCREL17C:
		val += IMM17(v32) + addend;
		v32 &= 0xffe0e002;
		v32 |= ASS17(val);
		break;

#define	IMM14(v)	((((v) & 0x3ffe) >> 1) | ((v) & 1? ~0x1fff : 0))
#define	ASS14(v)	((((v) & 0x1fff) << 1) | (((v) & 1) << 13))

	case RELOC_PCREL14R:
		val += IMM14(v32) + addend;
		val &= 0x7ff;
		v32 &= 0xffe0e002;
		v32 |= ASS14(val);
		break;

	case RELOC_DIR14R:
	case RELOC_DPREL14R:
	case RELOC_GPREL14R:
		val += IMM14(v32) + RND(addend) + (addend - RND(addend));
		val &= 0x7ff;
		v32 &= 0xffe0e002;
		v32 |= ASS14(val);
		break;

#define	IMM12(v)	((((v) & 4) << 10) | (((v) & 0x1ff8) >> 1) | \
	((v) & 1? ~0x1fff : 0))
#define	ASS12(v)	((((v) & 0x2000) >> 13) | (((v) & 0x1000) >> 11) | \
	(((v) & 0xffc) << 1))

	case RELOC_PCREL12F:
		/* addb/subb/comb/movb XXX must be a bug in gas(1) */
		val += IMM12(v32) + addend;
		v32 &= 0xffffe002;
		v32 |= ASS12(val);
		break;

	case RELOC_PLABEL32:
		break;

	case RELOC_DPREL14WR:
	case RELOC_DPREL14DR:
	default:
		errx(1, "unknown reloc type %d insn 0x%08x", type, v32);
	}
	v32 = htobe32(v32);
	memcpy(p, &v32, sizeof v32);

	return 0;
}
