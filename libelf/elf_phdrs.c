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
    "$ABSD: elf_phdrs.c,v 1.1 2014/07/17 15:42:09 mickey Exp $";
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

#ifdef __FreeBSD__
#define swap16	bswap16
#define swap32	bswap32
#define swap64	bswap64
#endif

#if ELFSIZE == 32
#define	swap_addr	swap32
#define	swap_off	swap32
#define	swap_sword	swap32
#define	swap_word	swap32
#define	swap_sxword	swap32
#define	swap_xword	swap32
#define	swap_half	swap16
#define	swap_quarter	swap16
#define	elf_load_phdrs	elf32_load_phdrs
#define	elf_save_phdrs	elf32_save_phdrs
#define	elf_fix_phdrs	elf32_fix_phdrs
#elif ELFSIZE == 64
#define	swap_addr	swap64
#define	swap_off	swap64
#ifdef __alpha__
#define	swap_sword	swap64
#define	swap_word	swap64
#else
#define	swap_sword	swap32
#define	swap_word	swap32
#endif
#define	swap_sxword	swap64
#define	swap_xword	swap64
#define	swap_half	swap32
#define	swap_quarter	swap16
#define	elf_load_phdrs	elf64_load_phdrs
#define	elf_save_phdrs	elf64_save_phdrs
#define	elf_fix_phdrs	elf64_fix_phdrs
#else
#error "Unsupported ELF class"
#endif

Elf_Phdr *
elf_load_phdrs(const char *fn, FILE *fp, off_t foff, const Elf_Ehdr *eh)
{
	Elf_Phdr *phdr;

	if (elf_checkoff(fn, fp, foff, (off_t)eh->e_phnum * eh->e_phentsize))
		return NULL;

	/* XXX we might as well induce some shnum&shentsize limits */
	if ((phdr = calloc(eh->e_phnum, eh->e_phentsize)) == NULL) {
		warn("calloc(%d, %d)", (int)eh->e_phnum, (int)eh->e_phentsize);
		return (NULL);
	}

	if (fseeko(fp, foff + eh->e_phoff, SEEK_SET)) {
		warn("%s: fseeko", fn);
		free(phdr);
		return (NULL);
	}

	if (fread(phdr, eh->e_phentsize, eh->e_phnum, fp) != eh->e_phnum) {
		warnx("%s: premature EOF", fn);
		free(phdr);
		return (NULL);
	}

	return (phdr);
}

int
elf_save_phdrs(const char *fn, FILE *fp, off_t foff, const Elf_Ehdr *eh,
    const Elf_Phdr *phdr)
{
	if (fseeko(fp, foff + eh->e_phoff, SEEK_SET)) {
		warn("%s: fseeko", fn);
		return (1);
	}

	if (fwrite(phdr, eh->e_phentsize, eh->e_phnum, fp) != eh->e_phnum) {
		warnx("%s: premature EOF", fn);
		return (1);
	}

	return (0);
}

int
elf_fix_phdrs(const Elf_Ehdr *eh, Elf_Phdr *phdr)
{
	int i;

	/* nothing to do */
	if (eh->e_ident[EI_DATA] == ELF_TARG_DATA)
		return (0);

	for (i = eh->e_phnum; i--; phdr++) {
		phdr->p_type = swap32(phdr->p_type);
		phdr->p_flags = swap32(phdr->p_flags);
		phdr->p_offset = swap_off(phdr->p_offset);
		phdr->p_vaddr = swap_addr(phdr->p_vaddr);
		phdr->p_paddr = swap_addr(phdr->p_paddr);
		phdr->p_filesz = swap_xword(phdr->p_filesz);
		phdr->p_memsz = swap_xword(phdr->p_memsz);
		phdr->p_align = swap_xword(phdr->p_align);
	}

	return (1);
}
