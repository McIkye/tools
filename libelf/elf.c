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
    "$ABSD: elf.c,v 1.33 2014/07/16 13:46:57 mickey Exp $";
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
#define	elf_fix_note	elf32_fix_note
#define	elf_load_phdrs	elf32_load_phdrs
#define	elf_save_phdrs	elf32_save_phdrs
#define	elf_fix_phdrs	elf32_fix_phdrs
#define	elf_fix_sym	elf32_fix_sym
#define	elf_fix_rel	elf32_fix_rel
#define	elf_fix_rela	elf32_fix_rela
#define	elf_shstrload	elf32_shstrload
#define	elf2nlist	elf32_2nlist
#define	elf_shn2type	elf32_shn2type
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
#define	elf_fix_note	elf64_fix_note
#define	elf_load_phdrs	elf64_load_phdrs
#define	elf_save_phdrs	elf64_save_phdrs
#define	elf_fix_phdrs	elf64_fix_phdrs
#define	elf_fix_sym	elf64_fix_sym
#define	elf_fix_rel	elf64_fix_rel
#define	elf_fix_rela	elf64_fix_rela
#define	elf_shstrload	elf64_shstrload
#define	elf2nlist	elf64_2nlist
#define	elf_shn2type	elf64_shn2type
#else
#error "Unsupported ELF class"
#endif

int
elf_fix_note(Elf_Ehdr *eh, Elf_Note *en)
{
	/* nothing to do */
	if (eh->e_ident[EI_DATA] == ELF_TARG_DATA)
		return (0);

	en->namesz = swap32(en->namesz);
	en->descsz = swap32(en->descsz);
	en->type = swap32(en->type);

	return (1);
}

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

int
elf_fix_rel(Elf_Ehdr *eh, Elf_Rel *rel)
{
	/* nothing to do */
	if (eh->e_ident[EI_DATA] == ELF_TARG_DATA)
		return (0);

	rel->r_offset = swap_addr(rel->r_offset);
	rel->r_info = swap_xword(rel->r_info);

	return (1);
}

int
elf_fix_rela(Elf_Ehdr *eh, Elf_RelA *rela)
{
	/* nothing to do */
	if (eh->e_ident[EI_DATA] == ELF_TARG_DATA)
		return (0);

	rela->r_offset = swap_addr(rela->r_offset);
	rela->r_info = swap_xword(rela->r_info);
	rela->r_addend = swap_sxword(rela->r_addend);

	return (1);
}

char *
elf_shstrload(const char *fn, FILE *fp, off_t foff, const Elf_Ehdr *eh,
    const Elf_Shdr *shdr)
{
	size_t shstrsize;
	char *shstr;

	if (!eh->e_shstrndx || eh->e_shstrndx >= eh->e_shnum) {
		warnx("%s: invalid header", fn);
		return NULL;
	}

	shstrsize = shdr[eh->e_shstrndx].sh_size;
	if (shstrsize == 0) {
		warnx("%s: no section name list", fn);
		return (NULL);
	}

	if ((shstr = malloc(shstrsize)) == NULL) {
		warn("malloc(%d)", (int)shstrsize);
		return (NULL);
	}

	if (fseeko(fp, foff + shdr[eh->e_shstrndx].sh_offset, SEEK_SET)) {
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
