/*
 * Copyright (c) 2003-2014 Michael Shalayeff
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
    "$ABSD: elf_sym.c,v 1.2 2014/07/18 13:30:44 mickey Exp $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <a.out.h>
#include <elf_abi.h>
#include "elfuncs.h"
#include "elfswap.h"

int
elf_fix_sym(const Elf_Ehdr *eh, Elf_Sym *sym)
{
	/* nothing to do */
	if (eh->e_ident[EI_DATA] == ELF_TARG_DATA)
		return (0);

	sym->st_name = swap32(sym->st_name);
	sym->st_shndx = swap16(sym->st_shndx);
	sym->st_value = swap_addr(sym->st_value);
	sym->st_size = swap_xword(sym->st_size);

	return (1);
}

int
elf_shn2type(const Elf_Ehdr *eh, u_int shn, const char *sn)
{
	switch (shn) {
	case SHN_MIPS_SUNDEFINED:
		if (eh->e_machine == EM_MIPS)
			return (N_UNDF | N_EXT);
		break;

	case SHN_UNDEF:
		return (N_UNDF | N_EXT);

	case SHN_ABS:
		return (N_ABS);

	case SHN_MIPS_ACOMMON:
		if (eh->e_machine == EM_MIPS)
			return (N_COMM);
		break;

	case SHN_MIPS_SCOMMON:
		if (eh->e_machine == EM_MIPS)
			return (N_COMM);
		break;

	case SHN_COMMON:
		return (N_COMM);

	case SHN_MIPS_TEXT:
		if (eh->e_machine == EM_MIPS)
			return (N_TEXT);
		break;

	case SHN_MIPS_DATA:
		if (eh->e_machine == EM_MIPS)
			return (N_DATA);
		break;

	default:
		/* beyond 8 a table-driven binsearch shall be used */
		if (sn == NULL)
			return (-1);
		else if (!strcmp(sn, ELF_TEXT))
			return (N_TEXT);
		else if (!strcmp(sn, ELF_RODATA))
			return (N_SIZE);
		else if (!strcmp(sn, ELF_DATA))
			return (N_DATA);
		else if (!strcmp(sn, ELF_SDATA))
			return (N_DATA);
		else if (!strcmp(sn, ELF_BSS))
			return (N_BSS);
		else if (!strcmp(sn, ELF_SBSS))
			return (N_BSS);
		else if (!strncmp(sn, ELF_GOT, sizeof(ELF_GOT) - 1))
			return (N_DATA);
		else if (!strncmp(sn, ELF_PLT, sizeof(ELF_PLT) - 1))
			return (N_DATA);
	}

	return (-1);
}

/*
 * Devise nlist's type from Elf_Sym.
 * XXX this task is done as well in libc and kvm_mkdb.
 */
int
elf2nlist(Elf_Sym *sym, const Elf_Ehdr *eh, const Elf_Shdr *shdr,
    const char *shstr, struct nlist *np)
{
	u_int stt;
	const char *sn;
	int type;

	if (sym->st_shndx < eh->e_shnum)
		sn = shstr + shdr[sym->st_shndx].sh_name;
	else
		sn = NULL;

	switch (stt = ELF_ST_TYPE(sym->st_info)) {
	case STT_NOTYPE:
	case STT_OBJECT:
		type = elf_shn2type(eh, sym->st_shndx, sn);
		if (type < 0) {
			if (sn == NULL)
				np->n_other = '?';
			else
				np->n_type = stt == STT_NOTYPE? N_COMM : N_DATA;
		} else {
			/* a hack for .rodata check (; */
			if (type == N_SIZE) {
				np->n_type = N_DATA;
				np->n_other = 'r';
			} else
				np->n_type = type;
		}
		break;

	case STT_FUNC:
		type = elf_shn2type(eh, sym->st_shndx, sn);
		np->n_type = type < 0? N_TEXT : type;
		if (type < 0)
			np->n_other = 't';
		if (ELF_ST_BIND(sym->st_info) == STB_WEAK) {
			np->n_type = N_INDR;
			np->n_other = 'W';
		} else if (sn != NULL && *sn != 0 &&
		    strcmp(sn, ELF_INIT) &&
		    strcmp(sn, ELF_TEXT) &&
		    strcmp(sn, ELF_FINI))	/* XXX GNU compat */
			np->n_other = '?';
		break;

	case STT_SECTION:
		type = elf_shn2type(eh, sym->st_shndx, NULL);
		if (type < 0)
			np->n_other = '?';
		else
			np->n_type = type;
		break;

	case STT_FILE:
		np->n_type = N_FN | N_EXT;
		break;

	case STT_PARISC_MILLI:
		if (eh->e_machine == EM_PARISC)
			np->n_type = N_TEXT;
		else
			np->n_other = '?';
		break;

	default:
		np->n_other = '?';
		break;
	}
	if ((np->n_type & N_TYPE) != N_UNDF &&
	    ELF_ST_BIND(sym->st_info) != STB_LOCAL) {
		np->n_type |= N_EXT;
		if (np->n_other)
			np->n_other = toupper(np->n_other);
	}

	np->n_value = sym->st_value;
	np->n_un.n_strx = sym->st_name;
	return (0);
}
