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
    "$ABSD: elf_header.c,v 1.3 2014/07/18 12:37:52 mickey Exp $";
#endif /* not lint */

#include <sys/param.h>
#include <stdio.h>
#include <elf_abi.h>
#include "elfuncs.h"
#include "elfswap.h"

int
elf_fix_header(Elf_Ehdr *eh)
{
	/* nothing to do */
	if (eh->e_ident[EI_DATA] == ELF_TARG_DATA)
		return (0);

	eh->e_type = swap16(eh->e_type);
	eh->e_machine = swap16(eh->e_machine);
	eh->e_version = swap32(eh->e_version);
	eh->e_entry = swap_addr(eh->e_entry);
	eh->e_phoff = swap_off(eh->e_phoff);
	eh->e_shoff = swap_off(eh->e_shoff);
	eh->e_flags = swap32(eh->e_flags);
	eh->e_ehsize = swap16(eh->e_ehsize);
	eh->e_phentsize = swap16(eh->e_phentsize);
	eh->e_phnum = swap16(eh->e_phnum);
	eh->e_shentsize = swap16(eh->e_shentsize);
	eh->e_shnum = swap16(eh->e_shnum);
	eh->e_shstrndx = swap16(eh->e_shstrndx);

	return (1);
}

int
elf_chk_header(Elf_Ehdr *eh)
{
	elf_fix_header(eh);

	if (eh->e_ehsize < sizeof(Elf_Ehdr) || !IS_ELF(*eh) ||
	    eh->e_ident[EI_CLASS] != ELFCLASS ||
	    eh->e_ident[EI_VERSION] != ELF_TARG_VER ||
	    eh->e_ident[EI_DATA] >= ELFDATANUM ||
	    eh->e_version != EV_CURRENT ||
	    (eh->e_type >= ET_NUM && ET_LOOS > eh->e_type) ||
	    (eh->e_phentsize && eh->e_phentsize < sizeof(Elf_Phdr)) ||
	    eh->e_shentsize < sizeof(Elf_Shdr) ||
	    eh->e_shstrndx > eh->e_shnum) {
		elf_fix_header(eh);
		return -1;
	}

	return 0;
}
