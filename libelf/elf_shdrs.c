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
    "$ABSD: elf_shdrs.c,v 1.4 2014/07/18 12:16:04 mickey Exp $";
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
#define	elf_load_shdrs	elf32_load_shdrs
#define	elf_save_shdrs	elf32_save_shdrs
#define	elf_scan_shdrs	elf32_scan_shdrs
#define	elf_fix_shdrs	elf32_fix_shdrs
#define	elf_fix_shdr	elf32_fix_shdr
#define	elf_shstrload	elf32_shstrload
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
#define	elf_load_shdrs	elf64_load_shdrs
#define	elf_save_shdrs	elf64_save_shdrs
#define	elf_scan_shdrs	elf64_scan_shdrs
#define	elf_fix_shdrs	elf64_fix_shdrs
#define	elf_fix_shdr	elf64_fix_shdr
#define	elf_shstrload	elf64_shstrload
#else
#error "Unsupported ELF class"
#endif

Elf_Shdr *
elf_load_shdrs(const char *fn, FILE *fp, off_t foff, const Elf_Ehdr *eh)
{
	Elf_Shdr *shdr;

	if (elf_checkoff(fn, fp, foff, (off_t)eh->e_phnum * eh->e_phentsize))
		return NULL;

	/* XXX we might as well induce some shnum&shentsize limits */
	if ((shdr = calloc(eh->e_shnum, eh->e_shentsize)) == NULL) {
		warn("calloc(%d, %d)", (int)eh->e_shnum, (int)eh->e_shentsize);
		return (NULL);
	}

	if (fseeko(fp, foff + eh->e_shoff, SEEK_SET)) {
		warn("%s: fseeko", fn);
		free(shdr);
		return (NULL);
	}

	if (fread(shdr, eh->e_shentsize, eh->e_shnum, fp) != eh->e_shnum) {
		warnx("%s: premature EOF", fn);
		free(shdr);
		return (NULL);
	}

	return (shdr);
}

int
elf_save_shdrs(const char *fn, FILE *fp, off_t foff, const Elf_Ehdr *eh,
    const Elf_Shdr *shdr)
{
	if (fseeko(fp, foff + eh->e_shoff, SEEK_SET)) {
		warn("%s: fseeko", fn);
		return (1);
	}

	if (fwrite(shdr, eh->e_shentsize, eh->e_shnum, fp) != eh->e_shnum) {
		warnx("%s: premature EOF", fn);
		return (1);
	}

	return (0);
}

int
elf_fix_shdr(Elf_Shdr *sh, const char *name)
{
	sh->sh_name = swap32(sh->sh_name);
	sh->sh_type = swap32(sh->sh_type);
	sh->sh_flags = swap_xword(sh->sh_flags);
	sh->sh_addr = swap_addr(sh->sh_addr);
	sh->sh_offset = swap_off(sh->sh_offset);
	sh->sh_size = swap_xword(sh->sh_size);
	sh->sh_link = swap32(sh->sh_link);
	sh->sh_info = swap32(sh->sh_info);
	sh->sh_addralign = swap_xword(sh->sh_addralign);
	sh->sh_entsize = swap_xword(sh->sh_entsize);

	return 1;
}

// XXX this shall be in elf.c

int
elf_fix_shdrs(const Elf_Ehdr *eh, Elf_Shdr *shdr)
{
	/* nothing to do */
	if (eh->e_ident[EI_DATA] == ELF_TARG_DATA)
		return (0);

	elf_scan_shdrs(eh, shdr, NULL, &elf_fix_shdr);

	return (1);
}

Elf_Shdr *
elf_scan_shdrs(const Elf_Ehdr *eh, Elf_Shdr *shdr, const char *shstr,
    int (*fn)(Elf_Shdr *, const char *))
{
	Elf_Shdr *esh;
	int i;

	esh = (Elf_Shdr *)((char *)shdr + eh->e_shnum * eh->e_shentsize);
	for (i = 0; shdr < esh; i++,
	    shdr = (Elf_Shdr *)((char *)shdr + eh->e_shentsize))
		if (!(*fn)(shdr, shstr + shdr->sh_name))
			return shdr;

	return NULL;
}

char *
elf_shstrload(const char *fn, FILE *fp, off_t foff, const Elf_Ehdr *eh,
    const Elf_Shdr *shdr)
{
	size_t shstrsize;
	char *shstr;

	if (!eh->e_shstrndx || eh->e_shstrndx >= eh->e_shnum) {
		warnx("%s: invalid ELF header", fn);
		return NULL;
	}
	shdr = (const Elf_Shdr *)((const char *)shdr +
	    eh->e_shstrndx * eh->e_shentsize);

	shstrsize = shdr->sh_size;
	if (shstrsize == 0) {
		warnx("%s: no section name list", fn);
		return (NULL);
	}

	if ((shstr = malloc(shstrsize)) == NULL) {
		warn("malloc(%d)", (int)shstrsize);
		return (NULL);
	}

	if (fseeko(fp, foff + shdr->sh_offset, SEEK_SET)) {
		warn("%s: fseeko", fn);
		free(shstr);
		return (NULL);
	}

	if (fread(shstr, shstrsize, 1, fp) != 1) {
		warnx("%s: premature EOF", fn);
		free(shstr);
		return (NULL);
	}

	return shstr;
}
