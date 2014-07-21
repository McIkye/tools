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
    "$ABSD: elf_shdrs.c,v 1.8 2014/07/21 13:57:53 mickey Exp $";
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
elf_fix_shdr(Elf_Shdr *sh, const char *name, void *v)
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

int
elf_fix_shdrs(const Elf_Ehdr *eh, Elf_Shdr *shdr)
{
	/* nothing to do */
	if (eh->e_ident[EI_DATA] == ELF_TARG_DATA)
		return (0);

	elf_scan_shdrs(eh, shdr, NULL, &elf_fix_shdr, NULL);

	return (1);
}

Elf_Shdr *
elf_scan_shdrs(const Elf_Ehdr *eh, Elf_Shdr *shdr, const char *shstr,
    int (*fn)(Elf_Shdr *, const char *, void *), void *v)
{
	Elf_Shdr *esh;
	int i;

	esh = (Elf_Shdr *)((char *)shdr + eh->e_shnum * eh->e_shentsize);
	for (i = 0; shdr < esh; i++,
	    shdr = (Elf_Shdr *)((char *)shdr + eh->e_shentsize))
		if (!(*fn)(shdr, shstr + shdr->sh_name, v))
			return shdr;

	return NULL;
}

char *
elf_shstrload(const char *fn, FILE *fp, off_t foff, const Elf_Ehdr *eh,
    const Elf_Shdr *shdr)
{
	if (!eh->e_shstrndx || eh->e_shstrndx >= eh->e_shnum) {
		warnx("%s: invalid ELF header", fn);
		return NULL;
	}
	shdr = (const Elf_Shdr *)((const char *)shdr +
	    eh->e_shstrndx * eh->e_shentsize);

	return elf_sld(fn, fp, foff, shdr);
}

char *
elf_sld(const char *fn, FILE *fp, off_t foff, const Elf_Shdr *shdr)
{
	size_t shsize;
	char *sld;

	if (shdr->sh_size == 0 || shdr->sh_size > SSIZE_MAX) {
		warnx("%s: no section name list", fn);
		return (NULL);
	}

	shsize = shdr->sh_size;
	if ((sld = malloc(shsize)) == NULL) {
		warn("malloc(%zd)", shsize);
		return (NULL);
	}

	if (fseeko(fp, foff + shdr->sh_offset, SEEK_SET)) {
		warn("%s: fseeko", fn);
		free(sld);
		return (NULL);
	}

	if (fread(sld, shsize, 1, fp) != 1) {
		warnx("%s: premature EOF", fn);
		free(sld);
		return (NULL);
	}

	return sld;
}
