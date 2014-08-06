/*
 * Copyright (c) 2014 Michael Shalayeff
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
    "$ABSD: elf_dwarfnebula.c,v 1.1 2014/08/08 16:36:09 mickey Exp $";
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <elf_abi.h>
#include <dwarf.h>
#include "elfuncs.h"
#include "elfswap.h"

/* ARGSUSED */
int
elf_lines_cmp(Elf_Shdr *sh, const char *name, void *v)
{
	return strcmp(name, v);
}

/*
 * pull in requested pieces of debug info
 */
struct dwarf_nebula *
elf_dwarfnebula(const char *name, FILE *fp, off_t foff, const Elf_Ehdr *eh,
    int flags)
{
	struct dwarf_nebula *dn = NULL;
	Elf_Shdr *shdr = NULL, *sh;
	char *shstr = NULL;
	char *names = NULL;
	char *lines = NULL;
	char *str = NULL;
	char *abbrv = NULL;
	char *info = NULL;

	if (!flags)
		return NULL;

	if (!(shdr = elf_load_shdrs(name, fp, foff, eh)))
		return NULL;

	elf_fix_shdrs(eh, shdr);
	if (!(shstr = elf_shstrload(name, fp, foff, eh, shdr)))
		return NULL;

	if (!(dn = calloc(1, sizeof *dn))) {
		warn("calloc");
		free(shstr);
		free(shdr);
		return NULL;
	}

	dn->name = name;
	dn->elfdata = eh->e_ident[EI_DATA];

	if (!(sh = elf_scan_shdrs(eh, shdr, shstr,
	    elf_lines_cmp, DWARF_INFO))) {
		warnx("%s: no " DWARF_INFO " section", name);
		goto kaput;
	}

	if (!(info = elf_sld(name, fp, foff, sh)))
		goto kaput;
	dn->ninfo = (ssize_t)sh->sh_size;

	if (!(sh = elf_scan_shdrs(eh, shdr, shstr,
	    elf_lines_cmp, DWARF_ABBREV))) {
		warnx("%s: no " DWARF_ABBREV " section", name);
		goto kaput;
	}

	if (!(abbrv = elf_sld(name, fp, foff, sh)))
		goto kaput;
	dn->nabbrv = (ssize_t)sh->sh_size;

	if (!(sh = elf_scan_shdrs(eh, shdr, shstr,
	    elf_lines_cmp, DWARF_STR))) {
		warnx("%s: no " DWARF_STR " section", name);
		goto kaput;
	}

	if (!(str = elf_sld(name, fp, foff, sh)))
		goto kaput;
	dn->nstr = (ssize_t)sh->sh_size;

	if (flags & ELF_DWARF_LINES) {
		if (!(sh = elf_scan_shdrs(eh, shdr, shstr,
		    elf_lines_cmp, DWARF_LINE))) {
			warnx("%s: no " DWARF_LINE " section", name);
			goto kaput;
		}

		if (!(lines = elf_sld(name, fp, foff, sh)))
			goto kaput;

		dn->nlines = (ssize_t)sh->sh_size;
	}

	if (flags & ELF_DWARF_NAMES) {
		if (!(sh = elf_scan_shdrs(eh, shdr, shstr,
		    elf_lines_cmp, DWARF_PUBNAMES))) {
			warnx("%s: no " DWARF_PUBNAMES " section", name);
			goto kaput;
		}

		if (!(names = elf_sld(name, fp, foff, sh)))
			goto kaput;

		dn->nnames = (ssize_t)sh->sh_size;
	}

	free(shstr);
	free(shdr);
	dn->names = names;
	dn->lines = lines;
	dn->str = str;
	dn->abbrv = abbrv;
	dn->info = info;
	if ((dn->nunits = dwarf_info_count(dn)) > 0) {
	    	if (flags & ELF_DWARF_LINES) {
			if (!dwarf_info_lines(dn))
				return dn;
		} else
			return dn;
	}
	if (flags & ELF_DWARF_LINES)
 kaput:
	free(shstr);
	free(shdr);
	free(names);
	free(lines);
	free(str);
	free(abbrv);
	free(info);
	free(dn);
	return NULL;
}
