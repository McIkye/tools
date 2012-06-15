/*
 * Copyright (c) 2003-2011 Michael Shalayeff
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
    "$ABSD: elf_size.c,v 1.1 2012/06/15 01:52:57 mickey Exp $";
#endif /* not lint */

#include <stdio.h>
#include <elf_abi.h>
#include "elfuncs.h"

#if ELFSIZE == 32
#define	elf_size	elf32_size
#elif ELFSIZE == 64
#define	elf_size	elf64_size
#else
#error "Unsupported ELF class"
#endif

int
elf_size(const Elf_Ehdr *eh, const Elf_Shdr *shdr,
    u_long *ptext, u_long *pdata, u_long *pbss)
{
	int i;

	*ptext = *pdata = *pbss = 0;

	for (i = 0; i < eh->e_shnum; i++) {
		if (!(shdr[i].sh_flags & SHF_ALLOC))
			continue;
		else if (shdr[i].sh_flags & SHF_EXECINSTR ||
		    !(shdr[i].sh_flags & SHF_WRITE))
			*ptext += (u_long)shdr[i].sh_size;
		else if (shdr[i].sh_type == SHT_NOBITS)
			*pbss += (u_long)shdr[i].sh_size;
		else
			*pdata += (u_long)shdr[i].sh_size;
	}

	return (0);
}
