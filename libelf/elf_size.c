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
    "$ABSD: elf_size.c,v 1.3 2014/07/18 13:45:36 mickey Exp $";
#endif /* not lint */

#include <stdio.h>
#include <elf_abi.h>
#include "elfuncs.h"
#include "elfswap.h"

struct elf_size {
	u_long text, data, bss;
};

int
elf_size_add(Elf_Shdr *sh, const char *name, void *v)
{
	struct elf_size *es = v;

	if (!(sh->sh_flags & SHF_ALLOC))
		return 1;
	else if (sh->sh_flags & SHF_EXECINSTR ||
	    !(sh->sh_flags & SHF_WRITE))
		es->text += (u_long)sh->sh_size;
	else if (sh->sh_type == SHT_NOBITS)
		es->bss += (u_long)sh->sh_size;
	else
		es->data += (u_long)sh->sh_size;

	return 1;
}

int
elf_size(const Elf_Ehdr *eh, Elf_Shdr *shdr,
    u_long *ptext, u_long *pdata, u_long *pbss)
{
	struct elf_size es = { 0, 0, 0 };

	elf_scan_shdrs(eh, shdr, NULL, elf_size_add, &es);

	*ptext = es.text;
	*pdata = es.data;
	*pbss = es.bss;
	return (0);
}
