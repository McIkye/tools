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
    "$ABSD: elf_symload.c,v 1.3 2014/07/18 13:30:44 mickey Exp $";
#endif /* not lint */

#include <sys/param.h>
#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf_abi.h>
#include "elfuncs.h"
#include "elfswap.h"

int elf_symloadx(struct elf_symtab *es, FILE *fp, off_t foff,
    int (*func)(struct elf_symtab *, int, void *, void *), void *arg,
    const char *, const char *);
char *elf_strload(const char *fn, FILE *fp, off_t foff, const Elf_Ehdr *eh,
    Elf_Shdr *shdr, const char *shstr, const char *strtab,
    size_t *pstabsize);

struct elf_stab_cmp {
	const char *name;
};

int
elf_stab_cmp(Elf_Shdr *sh, const char *name, void *v)
{
	struct elf_stab_cmp *n = v;

	return strcmp(name, n->name);
}

char *
elf_strload(const char *fn, FILE *fp, off_t foff, const Elf_Ehdr *eh,
    Elf_Shdr *shdr, const char *shstr, const char *strtab,
    size_t *pstabsize)
{
	struct elf_stab_cmp n;
	char *ret = NULL;

	n.name = strtab;
	if (!(shdr = elf_scan_shdrs(eh, shdr, shstr, elf_stab_cmp, &n)))
		return NULL;

	*pstabsize = shdr->sh_size;
	if (*pstabsize > SIZE_T_MAX) {
		warnx("%s: corrupt file", fn);
		return (NULL);
	}

	if ((ret = malloc(*pstabsize)) == NULL)
		warn("malloc(%zd)", *pstabsize);
	else if (pread(fileno(fp), ret, *pstabsize,
	    foff + shdr->sh_offset) != *pstabsize) {
		warn("pread: %s", fn);
		free(ret);
		ret = NULL;
	}

	return ret;
}

int
elf_symloadx(struct elf_symtab *es, FILE *fp, off_t foff,
    int (*func)(struct elf_symtab *, int, void *, void *), void *arg,
    const char *strtab, const char *symtab)
{
	const Elf_Ehdr *eh = es->ehdr;
	Elf_Shdr *shdr = es->shdr;
	struct elf_stab_cmp n;
	size_t symsize;
	Elf_Sym sbuf;
	int is;

	if (!(es->stab = elf_strload(es->name, fp, foff, eh, shdr, es->shstr,
	    strtab, &es->stabsz)))
		return (1);

	n.name = symtab;
	if (!(shdr = elf_scan_shdrs(eh, shdr, es->shstr, elf_stab_cmp, &n)))
		return (1);

	if (shdr->sh_entsize < sizeof sbuf) {
		warn("%s: invalid symtab section", es->name);
		return (1);
	}

	symsize = shdr->sh_size;
	if (fseeko(fp, foff + shdr->sh_offset, SEEK_SET)) {
		warn("%s: fseeko", es->name);
		return (1);
	}

	es->nsyms = symsize / shdr->sh_entsize;
	for (is = 0; is < es->nsyms; is++) {
		if (fread(&sbuf, sizeof sbuf, 1, fp) != 1) {
			warn("%s: read symbol", es->name);
			return (1);
		}
		if (shdr->sh_entsize > sizeof sbuf &&
		    fseeko(fp, shdr->sh_entsize - sizeof sbuf,
		    SEEK_CUR)) {
			warn("%s: fseeko", es->name);
			return (1);
		}

		elf_fix_sym(eh, &sbuf);
		if (sbuf.st_name >= es->stabsz)
			continue;

		if ((*func)(es, is, &sbuf, arg))
			return 1;
	}

	return (0);
}

int
elf_symload(struct elf_symtab *es, FILE *fp, off_t foff,
    int (*func)(struct elf_symtab *, int, void *, void *), void *arg)
{
	int rv;

	if (!es->shdr) {
		if (!(es->shdr = elf_load_shdrs(es->name, fp, foff, es->ehdr)))
			return 1;
		elf_fix_shdrs(es->ehdr, es->shdr);
	}

	if (!es->shstr && !(es->shstr = elf_shstrload(es->name, fp, foff,
	    es->ehdr, es->shdr)))
		return 1;

	es->stab = NULL;
	if (elf_symloadx(es, fp, foff, func, arg, ELF_STRTAB, ELF_SYMTAB)) {
		free(es->stab);
		es->stab = NULL;
		if ((rv = elf_symloadx(es, fp, foff, func, arg,
		    ELF_DYNSTR, ELF_DYNSYM))) {
			free(es->stab);
			return rv;
		}
	}

	return (0);
}
