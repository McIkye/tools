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
    "$ABSD: elf_phdrs.c,v 1.3 2014/07/18 13:01:13 mickey Exp $";
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
elf_fix_phdr(Elf_Phdr *ph)
{
	ph->p_type = swap32(ph->p_type);
	ph->p_flags = swap32(ph->p_flags);
	ph->p_offset = swap_off(ph->p_offset);
	ph->p_vaddr = swap_addr(ph->p_vaddr);
	ph->p_paddr = swap_addr(ph->p_paddr);
	ph->p_filesz = swap_xword(ph->p_filesz);
	ph->p_memsz = swap_xword(ph->p_memsz);
	ph->p_align = swap_xword(ph->p_align);

	return 1;
}

int
elf_fix_phdrs(const Elf_Ehdr *eh, Elf_Phdr *phdr)
{
	/* nothing to do */
	if (eh->e_ident[EI_DATA] == ELF_TARG_DATA)
		return (0);

	elf_scan_phdrs(eh, phdr, elf_fix_phdr);

	return (1);
}


Elf_Phdr *
elf_scan_phdrs(const Elf_Ehdr *eh, Elf_Phdr *phdr, int (*fn)(Elf_Phdr *))
{
	Elf_Phdr *eph;
	int i;

	eph = (Elf_Phdr *)((char *)phdr + eh->e_phnum * eh->e_phentsize);
	for (i = 0; phdr < eph; i++,
	    phdr = (Elf_Phdr *)((char *)phdr + eh->e_phentsize))
		if (!(*fn)(phdr))
			return phdr;

	return NULL;
}
