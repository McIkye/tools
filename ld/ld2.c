/*
 * Copyright (c) 2009-2014 Michael Shalayeff
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
    "$ABSD: ld2.c,v 1.45 2013/10/24 09:46:41 mickey Exp $";
#endif

#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf_abi.h>
#include <elfuncs.h>
#include <a.out.h>
#include <nlist.h>
#include <errno.h>
#include <err.h>

#include "ld.h"

/* some of these also duplicate libelf/elf.c */
#if ELFSIZE == 32
#define	ELF_ADDRALIGN	4
#define	ELF_HDR(h)	((h).elf32)
#define	ELF_SYM(h)	((h).sym32)
#define	uLD		uLD32
#define	ldorder_obj	ld32order_obj
#define	ldmap		ldmap32
#define	ldload		ldload32
#define	ldloadasect	ldloadasect32
#define	elf_ld_chkhdr	elf32_ld_chkhdr
#define	elf_fix_header	elf32_fix_header
#define	elf_fix_note	elf32_fix_note
#define	elf_fix_phdrs	elf32_fix_phdrs
#define	elf_fix_shdrs	elf32_fix_shdrs
#define	elf_fix_sym	elf32_fix_sym
#define	elf_fix_rel	elf32_fix_rel
#define	elf_fix_rela	elf32_fix_rela
#define	elf2nlist	elf32_2nlist
#define	elf_load_shdrs	elf32_load_shdrs
#define	elf_save_shdrs	elf32_save_shdrs
#define	elf_save_phdrs	elf32_save_phdrs
#define	elf_fix_shdrs	elf32_fix_shdrs
#define	elf_symload	elf32_symload
#define	elf_loadrelocs	elf32_loadrelocs
#define	elf_addreloc	elf32_addreloc
#define	elf_absadd	elf32_absadd
#define	elf_symadd	elf32_symadd
#define	elf_objadd	elf32_objadd
#define	elf_commons	elf32_commons
#define	elf_symprintmap	elf32_symprintmap
#define	elf_symrec	elf32_symrec
#define	elf_symwrite	elf32_symwrite
#define	elf_names	elf32_names
#define	elf_prefer	elf32_prefer
#define	elf_seek	elf32_seek
#elif ELFSIZE == 64
#define	ELF_ADDRALIGN	8
#define	ELF_HDR(h)	((h).elf64)
#define	ELF_SYM(h)	((h).sym64)
#define	uLD		uLD64
#define	ldorder_obj	ld64order_obj
#define	ldmap		ldmap64
#define	ldload		ldload64
#define	ldloadasect	ldloadasect64
#define	elf_ld_chkhdr	elf64_ld_chkhdr
#define	elf_fix_header	elf64_fix_header
#define	elf_fix_note	elf64_fix_note
#define	elf_fix_phdrs	elf64_fix_phdrs
#define	elf_fix_shdrs	elf64_fix_shdrs
#define	elf_fix_sym	elf64_fix_sym
#define	elf_fix_rel	elf64_fix_rel
#define	elf_fix_rela	elf64_fix_rela
#define	elf2nlist	elf64_2nlist
#define	elf_load_shdrs	elf64_load_shdrs
#define	elf_save_shdrs	elf64_save_shdrs
#define	elf_save_phdrs	elf64_save_phdrs
#define	elf_fix_shdrs	elf64_fix_shdrs
#define	elf_symload	elf64_symload
#define	elf_loadrelocs	elf64_loadrelocs
#define	elf_addreloc	elf64_addreloc
#define	elf_absadd	elf64_absadd
#define	elf_symadd	elf64_symadd
#define	elf_objadd	elf64_objadd
#define	elf_commons	elf64_commons
#define	elf_symprintmap	elf64_symprintmap
#define	elf_symrec	elf64_symrec
#define	elf_symwrite	elf64_symwrite
#define	elf_names	elf64_names
#define	elf_prefer	elf64_prefer
#define	elf_seek	elf64_seek
#else
#error "Unsupported ELF class"
#endif

int elf_commons(struct objlist *, void *);

int elf_symrec(const struct ldorder *, const struct section *,
    struct symlist *, void *);
int elf_names(const struct ldorder *, const struct section *,
    struct symlist *, void *);
int elf_symprintmap(const struct ldorder *, const struct section *,
    struct symlist *, void *);
int elf_symwrite(const struct ldorder *, const struct section *,
    struct symlist *, void *);
Elf_Off elf_prefer(Elf_Off, struct ldorder *, uint64_t);
int elf_seek(FILE *, off_t, uint64_t);
int elf_addreloc(struct objlist *, struct section *, struct relist *,
    Elf_Rel *, uint64_t);

/*
 * map all the objects into the loading order;
 * since that is done -- print out the map and the xref table;
 * NB maybe this can be moved to the ld.c
 */
struct ldorder *
ldmap(struct headorder *headorder)
{
	struct ldorder *ord, *next;
	struct section *os;
	struct symlist *sym;
	Elf_Ehdr *eh;
	Elf_Phdr *phdr;
	Elf_Shdr *shdr;
	Elf_Sym *esym;
	uint64_t point, align;
	Elf_Off off;
	int nsect, nphdr, nnames[2];

	eh = &ELF_HDR(sysobj.ol_hdr);
	eh->e_ident[EI_MAG0] = ELFMAG0;
	eh->e_ident[EI_MAG1] = ELFMAG1;
	eh->e_ident[EI_MAG2] = ELFMAG2;
	eh->e_ident[EI_MAG3] = ELFMAG3;
	eh->e_ident[EI_CLASS] = elfclass;
	eh->e_ident[EI_DATA] = endian;
	eh->e_ident[EI_VERSION] = EV_CURRENT;
	eh->e_ident[EI_OSABI] = ELFOSABI_SYSV;
	eh->e_ident[EI_ABIVERSION] = 0;
	eh->e_type = relocatable? ET_REL : Bflag == 2? ET_DYN : ET_EXEC;
	eh->e_machine = machine;
	eh->e_version = EV_CURRENT;

	/* assign commons */
	obj_foreach(elf_commons, NULL);

	if (gc_sections)
		headorder = elf_gcs(headorder);

	/*
	 * stroll through the order counting {e,p,s}hdrs;
	 */
	shdr = sysobj.ol_sects;
	sysobj.ol_sections[0].os_sect = shdr++;
	for (nsect = 1, nphdr = 0, ord = TAILQ_FIRST(headorder);
	    ord != TAILQ_END(headorder); ord = next) {

		next = TAILQ_NEXT(ord, ldo_entry);
		if (ord->ldo_order == ldo_symbol ||
		    ord->ldo_order == ldo_expr)
			continue;

		if (ord->ldo_order == ldo_symtab) {
			/*
			 * at this point we cannot have any more
			 * symbols defined and thus can generate the strtab
			 */
			/* count names and assign st_names */
			nnames[0] = 0;
			nnames[1] = 1;
			sym_scan(TAILQ_FIRST(headorder), NULL, elf_symrec,
			    nnames);
			ord->ldo_wsize = nnames[0] + sizeof *esym;
		} else if (ord->ldo_order == ldo_strtab) {
			ord->ldo_start = 0;
			ord->ldo_addr = 1;
			ord->ldo_wsize = ALIGN(nnames[1]);
			if (!(ord->ldo_wurst = malloc(ord->ldo_wsize)))
				err(1, "malloc");
			*(char *)ord->ldo_wurst = '\0';
			sym_scan(TAILQ_FIRST(headorder), NULL, elf_names, ord);
		} else if (ord->ldo_order == ldo_expr)
			continue;

		/* skip empty sections */
		if (TAILQ_EMPTY(&ord->ldo_seclst) &&
		    !((ord->ldo_flags & LD_CONTAINS) && ord->ldo_wsize)) {
			TAILQ_REMOVE(headorder, ord, ldo_entry);
			continue;
		}

		sysobj.ol_sections[nsect].os_sect = shdr;
		ord->ldo_sect = &sysobj.ol_sections[nsect];

		ord->ldo_sno = nsect++;
		shdr->sh_type = ord->ldo_type;
		shdr->sh_flags = ord->ldo_shflags;
		shdr->sh_addralign = ELF_ADDRALIGN;
		if (ord->ldo_type == SHT_SYMTAB ||
		    ord->ldo_type == SHT_DYNSYM) {
			shdr->sh_link = nsect;
			shdr->sh_entsize = sizeof *esym;
		}

		if (shdr->sh_flags & SHF_ALLOC &&
		    (shdr->sh_type != shdr[-1].sh_type ||
		     shdr->sh_flags != shdr[-1].sh_flags))
			nphdr++;

		shdr++;
	}

	if (nsect == 1 || !nphdr)
		errx(1, "output headers botch");
	sysobj.ol_nsect = nsect;

	if (!(phdr = calloc(nphdr, sizeof *phdr)))
		err(1, "calloc");
	sysobj.ol_aux = phdr;

	eh->e_phoff = sizeof *eh;
	eh->e_flags = 0;
	eh->e_ehsize = sizeof *eh;
	eh->e_phentsize = sizeof *phdr;
	eh->e_phnum = nphdr;
	eh->e_shentsize = sizeof *shdr;
	eh->e_shnum = sysobj.ol_nsect;

	off = sizeof *eh + nphdr * sizeof *phdr;
	point = (arc4random() & 0xffffff) + off;
	TAILQ_FOREACH(ord, headorder, ldo_entry) {

		shdr = NULL;
		switch (ord->ldo_order) {
		/* these already been handled in ldinit and/or ldorder */
		case ldo_option:
			break;

		case ldo_expr:
			/* mind the gap! */
			if (magic != ZMAGIC)
				point += __LDPGSZ;
			else
				point += (arc4random() & 0xffffff) + __LDPGSZ;
			break;

		case ldo_section:
			/* this is the output section header */
			shdr = ord->ldo_sect->os_sect;
			shdr->sh_offset = off = elf_prefer(off, ord, point);

			ord->ldo_start = ord->ldo_addr;
			/* roll thru all sections and map the symbols */
			TAILQ_FOREACH(os, &ord->ldo_seclst, os_entry) {
				shdr = os->os_sect;
				align = shdr->sh_addralign;
				if (align) {
					ord->ldo_addr += align - 1;
					ord->ldo_addr &= ~(align - 1);
				}
				shdr->sh_addr = ord->ldo_addr;
				if (shdr->sh_type != SHT_NOBITS)
					shdr->sh_offset = off +
					    ord->ldo_addr - ord->ldo_start;

				/* assign addrs to all syms in this section */
				TAILQ_FOREACH(sym, &os->os_syms, sl_entry) {
					esym = &ELF_SYM(sym->sl_elfsym);
					esym->st_value += shdr->sh_addr;
				}
				ord->ldo_addr += shdr->sh_size;
			}
			point = ord->ldo_addr;
			shdr = ord->ldo_sect->os_sect;
			if (shdr->sh_type != SHT_NOBITS)
				off += ord->ldo_addr - ord->ldo_start;
			break;

		case ldo_symbol:
			sym = ord->ldo_wurst;
			esym = &ELF_SYM(sym->sl_elfsym);
			esym->st_value =
			ord->ldo_start =
			ord->ldo_addr = point;
			break;

		case ldo_interp:
		case ldo_ehfrh:
		case ldo_symtab:
		case ldo_strtab:
			/* this is the output section header */
			shdr = ord->ldo_sect->os_sect;
			ord->ldo_start =
			ord->ldo_addr = point;
			point =
			ord->ldo_addr += ord->ldo_wsize;
			shdr->sh_offset = off;
			off += ord->ldo_addr - ord->ldo_start;
			break;

		/* following ord generate .shstr and the sh_name's */
		case ldo_shstr: {
			struct ldorder *sord;
			char *p, *q;

			/* this is the output section header */
			shdr = ord->ldo_sect->os_sect;
			ord->ldo_start = point;
			point += ord->ldo_wsize;
			ord->ldo_addr = point;
			shdr->sh_offset = off;
			off += ord->ldo_addr - ord->ldo_start;

			eh->e_shstrndx = ord->ldo_sno;
			eh->e_shoff = off = SHALIGN(off);
			off += sysobj.ol_nsect * sizeof *shdr;

			p = q = ord->ldo_wurst;
			*p++ = '\0';
			TAILQ_FOREACH(sord, headorder, ldo_entry) {
				size_t n;

				if (sord->ldo_order == ldo_symbol ||
				    sord->ldo_order == ldo_expr)
					continue;

				shdr = sord->ldo_sect->os_sect;
				shdr->sh_name = p - q;
				n = sord->ldo_wsize - (p - q);
				p += strlcpy(p, sord->ldo_name, n) + 1;
			}
			shdr = ord->ldo_sect->os_sect;
			break;
		}

		case ldo_kaput:
			err(1, "kaputziener");
		}

		if (shdr && (shdr->sh_flags & SHF_ALLOC)) {
			if (shdr->sh_type != shdr[-1].sh_type ||
			    shdr->sh_flags != shdr[-1].sh_flags) {
				if (shdr[-1].sh_type != SHT_NULL) {
					if (shdr[-1].sh_type != SHT_NOBITS)
						phdr->p_filesz = phdr->p_memsz;
					phdr++;
				}
				switch (shdr->sh_type) {
				case SHT_NOTE:     phdr->p_type = PT_NOTE; break;
				case SHT_DYNAMIC:  phdr->p_type = PT_DYNAMIC; break;
				case SHT_PROGBITS: phdr->p_type = PT_LOAD; break;
				case SHT_NOBITS:   phdr->p_type = PT_LOAD; break;
				}
				phdr->p_flags = PF_R;
				if (shdr->sh_flags & SHF_WRITE)
					phdr->p_flags |= PF_W;
				if (shdr->sh_flags & SHF_EXECINSTR)
					phdr->p_flags |= PF_X;
				phdr->p_offset = shdr->sh_offset;
				phdr->p_vaddr = ord->ldo_start;
				phdr->p_paddr = ord->ldo_start;
				phdr->p_memsz = ord->ldo_addr - ord->ldo_start;
				if (shdr->sh_type != SHT_NOBITS)
					phdr->p_filesz = phdr->p_memsz;
				phdr->p_align = phdr->p_type == PT_LOAD?
				    __LDPGSZ : 4;
			} else
				phdr->p_memsz = ord->ldo_addr - phdr->p_vaddr;
		}
	}

	if (printmap)
		sym_printmap(headorder, order_printmap, elf_symprintmap);

	if (sym_undcheck())
		return NULL;

	/* save the entry point address */
	ELF_HDR(sysobj.ol_hdr).e_entry = ELF_SYM(sentry->sl_elfsym).st_value;

	return TAILQ_FIRST(headorder);
}

/*
 * off had been calculated before and now only adjust the addr;
 * align offset of the section in a.out according to [randomised]
 * section load address
 *
 * XXX use of __LDPGSZ breaks cross-linking!!!
 */
Elf_Off
elf_prefer(Elf_Off off, struct ldorder *ord, uint64_t point)
{
	uint64_t align, a;
	Elf_Shdr *shdr = ord->ldo_sect->os_sect;
	Elf_Off o;

	if (start_text && !strcmp(ord->ldo_name, ".text"))
		point = start_text;
	else if (start_data && !strcmp(ord->ldo_name, ".data"))
		point = start_data;
	else if (start_bss && !strcmp(ord->ldo_name, ".bss"))
		point = start_bss;
	ord->ldo_addr = point;
	align = shdr->sh_addralign;
	ord->ldo_addr += align - 1;
	ord->ldo_addr &= ~(align - 1);
	if (shdr->sh_type != SHT_NOBITS)
		off += ord->ldo_addr - point;

	a = ord->ldo_addr & (__LDPGSZ - 1);
	o = off & (__LDPGSZ - 1);
	if (o < a)
		off += a - o;
	else if (o > a)
		off += __LDPGSZ - (o - a);

	return off;
}

/*
 * add an absolute symbol most likely
 * defined at the command line
 */
struct symlist *
elf_absadd(const char *name, int bind)
{
	struct symlist *sym;
	Elf_Sym asym;

	asym.st_name = 0;
	asym.st_value = 0;
	asym.st_size = 0;
	asym.st_info = ELF_ST_INFO(bind, STT_NOTYPE);
	asym.st_other = 0;
	asym.st_shndx = SHN_ABS;
	if ((sym = sym_isundef(name)))
		sym = sym_define(sym, NULL, &asym);
	else
		sym = sym_add(name, NULL, &asym);

	return sym;
}

/*
 * select needed section from the object for this order
 */
int
ldorder_obj(struct objlist *ol, void *v)
{
	struct ldorder *neworder = v;
	Elf_Shdr *shdr = ol->ol_sects;
	struct section *os = ol->ol_sections;
	int i, n;

	/* skip empty objects */
	if (!os)
		return 0;

	n = ol->ol_nsect;
	for (i = 0; i < n; i++, os++, shdr++) {
		if (shdr->sh_type == SHT_NULL)
			continue;

		/*
		 * fuzzy match on section names;
		 * this means .rodata will also pull .rodata.str
		 * and .bss will pull .bss.emergency_buffer ...
		 */
		if (strncmp(os->os_name, neworder->ldo_name,
		    strlen(neworder->ldo_name)))
			continue;

		/*
TODO
		 * aditionally shuffle the sections of the same kind to provide
		 * randomness to the symbol offsets; one can argue that
		 * debugging would be hell due to heisenbugs elluding their
		 * beholder but contrary to that by defixing their appearances
		 * we will pin on their vectors way more easily.
		 */
		os->os_flags |= SECTION_ORDER;
		/* check if we already got one of these */
		if (neworder->ldo_flags & LD_LINK1) {
			struct section *ss;
			/* TODO optimise linear search */
			TAILQ_FOREACH(ss, &neworder->ldo_seclst, os_entry)
				if (!strcmp(os->os_name, ss->os_name))
					break;
			if (ss != TAILQ_END(&neworder->ldo_seclst))
				continue;
		}
		TAILQ_INSERT_TAIL(&neworder->ldo_seclst, os, os_entry);
		if (neworder->ldo_flags & LD_USED)
			os->os_flags |= SECTION_USED;
	}

	return 0;
}

/*
 * called upon every symbol in order to determine
 * the needs in the string table; calculate the sym name pointers
 */
int
elf_symrec(const struct ldorder *order, const struct section *os,
    struct symlist *sym, void *v)
{
	Elf_Sym *esym;
	int *pnn = v;

	esym = &ELF_SYM(sym->sl_elfsym);
	pnn[0] += sizeof *esym;
	esym->st_name = pnn[1];
	pnn[1] += strlen(sym->sl_name) + 1;

	return 0;
}

/*
 * called upon every symbol in order to write out each one of them
 */
int
elf_symwrite(const struct ldorder *order, const struct section *os,
    struct symlist *sym, void *v)
{
	Elf_Ehdr *eh = &ELF_HDR(sysobj.ol_hdr);
	FILE *fp = v;
	Elf_Sym osym;

	osym = ELF_SYM(sym->sl_elfsym);
	osym.st_shndx = order->ldo_sno;
	elf_fix_sym(eh, &osym);
	if (fwrite(&osym, sizeof osym, 1, fp) != 1)
		err(1, "fwrite: symbol table");

	return 0;
}

/*
 * called upon every output symbol in order to write out the strings
 *
 * TODO merge equal strings (make string hash in syms.c)
 */
int
elf_names(const struct ldorder *order, const struct section *os,
    struct symlist *sym, void *v)
{
	struct ldorder *neworder = v;
	char *p = neworder->ldo_wurst;
	size_t n;

	p += neworder->ldo_addr;
	n = neworder->ldo_wsize - neworder->ldo_addr;
	neworder->ldo_addr += strlcpy(p, sym->sl_name, n) + 1;

	return 0;
}

/*
 * called upon every output symbol in order to produce
 * the a.out file map
 */
int
elf_symprintmap(const struct ldorder *order, const struct section *os,
    struct symlist *sym, void *v)
{
	static const struct section *prevos;
	Elf_Sym *esym = &ELF_SYM(sym->sl_elfsym);

	if (os != prevos) {
#if ELFSIZE == 32
		fprintf(v, "%-16s   %8s\t%s\n", "", "",
		    sym->sl_sect->os_obj->ol_name);
#else
		fprintf(v, "%-16s   %16s\t%s\n", "", "",
		    sym->sl_sect->os_obj->ol_name);
#endif
		prevos = os;
	}

#if ELFSIZE == 32
	fprintf(v, "%-16s 0x%08lx\t\t%s\n", "",
	    (u_long)esym->st_value, sym->sl_name);
#else
	fprintf(v, "%-16s 0x%016lx\t\t%s\n", "",
	    (u_long)esym->st_value, sym->sl_name);
#endif

	return 0;
}

/*
 * allocate commons
 */
int
elf_commons(struct objlist *ol, void *v)
{
	struct symlist *sym;
	Elf_Shdr *shdr;
	Elf_Sym *esym;

	if (!ol->ol_bss) {
		if (TAILQ_EMPTY(&ol->ol_sections[0].os_syms))
			return 0;
		errx(1, "%s: no .bss", ol->ol_name);
	}

	shdr = ol->ol_bss->os_sect;
	while ((sym = TAILQ_FIRST(&ol->ol_sections[0].os_syms))) {
		esym = &ELF_SYM(sym->sl_elfsym);
		esym->st_value = shdr->sh_size;
		shdr->sh_size += ALIGN(esym->st_size);
		sym_redef(sym, ol->ol_bss, NULL);
	}

	return 0;
}

/*
 * produce actual a.out
 * scan through the orders and sections messing the bits
 * and the headers; give special treatment to the symtab
 */
int
ldload(const char *name, struct ldorder *order)
{
	struct stat sb;
	char obuf[ELF_OBUFSZ];
	struct ldorder *ord;
	struct section *os;
	Elf_Ehdr *eh;
	Elf_Phdr *phdr;
	Elf_Shdr *shdr;
	FILE *fp;

	if (!order || errors)
		return 1;

	eh = &ELF_HDR(sysobj.ol_hdr);

	if (!(fp = fopen(name, "w+")))
		err(1, "fopen: %s", name);
	setvbuf(fp, obuf, _IOFBF, sizeof obuf);

	/* dump out sections */
	for (ord = order; ord != TAILQ_END(ord);
	    ord = TAILQ_NEXT(ord, ldo_entry)) {
		const char *inname = NULL;
		FILE *sfp = NULL;

		if (ord->ldo_order == ldo_symbol ||
		    ord->ldo_order == ldo_expr)
			continue;

		shdr = ord->ldo_sect->os_sect;
		if (shdr->sh_flags & SHF_ALLOC)
			shdr->sh_addr = ord->ldo_start;
		shdr->sh_size = ord->ldo_addr - ord->ldo_start;

		/* nothing to write */
		if (ord->ldo_type == SHT_NOBITS)
			continue;

		if (elf_seek(fp, shdr->sh_offset, ord->ldo_filler) < 0)
			err(1, "elf_seek: %s", name);

		/* done w/ meat -- generate symbols */
		if (ord->ldo_type == SHT_SYMTAB) {
			if (fseek(fp, sizeof(Elf_Sym), SEEK_CUR) < 0)
				err(1, "fseek: %s", name);
			sym_scan(order, NULL, elf_symwrite, fp);
			continue;
		}

		if (ord->ldo_flags & LD_CONTAINS) {
			if (fwrite(ord->ldo_wurst, ord->ldo_wsize, 1, fp) != 1)
				err(1, "fwrite: %s", name);

			continue;
		}

		/*
		 * scan thru the sections alike and dump 'em all out
		 * into the a.out;
		 * either dump per object or per order depending
		 * what is more of we have moderated by the open
		 * file limit.
		 */
		TAILQ_FOREACH(os, &ord->ldo_seclst, os_entry) {
			struct section *osp, *esp;

			if (inname != os->os_obj->ol_path) {
				if (sfp)
					fclose(sfp);
				inname = os->os_obj->ol_path;
				if (!(sfp = fopen(inname, "r")))
					err(1, "fopen: %s", inname);
			}

			/*
			 * here we cheat and try to do less fopen()s
			 * than fseeko()s by dumping all sections from
			 * one object with one fopen!
			 */
// TODO currently needs offsets calculated beforehands...
//			for (osp = os->os_obj->ol_sections,
//			     esp = osp + os->os_obj->ol_nsect;
//			     osp && osp < esp; osp++) {
//				if (!os->os_sect)
//					continue;
osp = os;
				if (!(osp->os_flags & SECTION_LOADED) &&
				    ldloadasect(sfp, fp, name, ord, osp))
					return -1;
				else
					osp->os_flags |= SECTION_LOADED;
//			}
		}
	}

	shdr = sysobj.ol_sects;
	elf_fix_shdrs(eh, shdr);
	if (elf_save_shdrs(name, fp, 0, eh, shdr))
		return -1;

	phdr = sysobj.ol_aux;
	elf_fix_phdrs(eh, phdr);
	if (elf_save_phdrs(name, fp, 0, eh, phdr))
		return -1;

	rewind(fp);

	elf_fix_header(eh);
	if (fwrite(eh, sizeof *eh, 1, fp) != 1)
		err(1, "fwrite: %s", name);

	if (fstat(fileno(fp), &sb))
		err(1, "stat: %s", name);

	sb.st_mode |= (S_IXUSR|S_IXGRP|S_IXOTH) & ~umask(0);
	if (fchmod(fileno(fp), sb.st_mode))
		err(1, "fchmod: %s", name);

	fclose(fp);

	/* rejoice */
	return 0;
}

/*
 * load one section from one object fixing relocs;
 * luckily we might fit all data in one buffer and loops are small (;
 */
int
ldloadasect(FILE *fp, FILE *ofp, const char *name, const struct ldorder *ord,
    struct section *os)
{
	char sbuf[ELF_SBUFSZ];
	off_t off, sl;
	Elf_Shdr *shdr = os->os_sect;
	int len, bof;

	if (elf_seek(ofp, shdr->sh_offset, ord->ldo_filler) < 0)
		err(1, "elf_seek: %s", name);

	if (fseeko(fp, os->os_off, SEEK_SET) < 0)
		err(1, "fseeko: %s", os->os_obj->ol_name);

	for (sl = shdr->sh_size, off = 0, bof = 0; off < sl; off += len) {
		len = sizeof sbuf - bof;
		if (len > sl - off)
			len = sl - off;
		if (fread(sbuf + bof, len, 1, fp) != 1) {
			if (feof(fp))
				errx(1, "fread: %s: EOF", os->os_obj->ol_name);
			else
				err(1, "fread: %s", os->os_obj->ol_name);
		}
		len += bof;
		bof = ord->ldo_arch->la_fix(off, os, sbuf, len);
		if (bof < 0 || bof >= ELF_SBUFSZ)
			return -1;
		else {
			if (fwrite(sbuf, len - bof, 1, ofp) != 1)
				err(1, "fwrite: %s", name);
			if (bof) {
				len -= bof;
				memcpy(sbuf, sbuf + len, bof);
			}
		}
	}

	return (0);
}

/*
 * load the relocations for the section;
 * resolve symbol references and sort out relocs
 * according to the address as required by the loader
 */
int
elf_loadrelocs(struct objlist *ol, struct section *os, Elf_Shdr *shdr,
    FILE *fp, off_t foff)
{
	off_t off;
	Elf_Ehdr *eh = &ELF_HDR(ol->ol_hdr);
	struct relist *r;
	int i, n, sz;

	off = ftello(fp);
	if (fseeko(fp, foff + shdr->sh_offset, SEEK_SET) < 0)
		err(1, "fseeko: %s", ol->ol_path);

	sz = shdr->sh_type == SHT_REL? sizeof(Elf_Rel) : sizeof(Elf_RelA);
	if (sz > shdr->sh_entsize)
		errx(1, "%s: corrupt elf header", ol->ol_path);
	sz = shdr->sh_entsize - sz;
	n = shdr->sh_size / shdr->sh_entsize;
	if (!(r = calloc(n, sizeof *r)))
		err(1, "calloc");

	os->os_rels = r;
	os->os_nrls = n;
	if (shdr->sh_type == SHT_REL) {
		Elf_Rel rel;

		for (i = 0; i < n; i++, r++) {
			if (fread(&rel, sizeof rel, 1, fp) != 1)
				err(1, "fread: %s", ol->ol_path);

			if (sz && fseeko(fp, sz, SEEK_CUR))
				err(1, "fseeko: %s", ol->ol_path);

			elf_fix_rel(eh, &rel);
			elf_addreloc(ol, os, r, &rel, 0);
		}
	} else {
		Elf_RelA rela;

		for (i = 0; i < n; i++, r++) {
			if (fread(&rela, sizeof rela, 1, fp) != 1)
				err(1, "fread: %s", ol->ol_path);

			if (sz && fseeko(fp, sz, SEEK_CUR))
				err(1, "fseeko: %s", ol->ol_path);

			elf_fix_rela(eh, &rela);
			/* we assume that r_addend is added last */
			elf_addreloc(ol, os, r,
			    (Elf_Rel *)&rela, rela.r_addend);
		}
	}

	if (fseeko(fp, off, SEEK_SET) < 0)
		err(1, "fseeko: %s", ol->ol_path);

	/* we gotta sort them by addr as they come unsorted */
	qsort(os->os_rels, os->os_nrls, sizeof *os->os_rels, rel_addrcmp);

	return 0;
}

int
elf_addreloc(struct objlist *ol, struct section *os, struct relist *r,
    Elf_Rel *rel, uint64_t addend)
{
	struct symlist *sym, **sidx = ol->ol_aux;
	Elf_Shdr *shbits = os->os_sect;
	Elf_Sym *esym;
	const char *snam;
	int si;

	if ((si = ELF_R_SYM(rel->r_info)) >= ol->ol_naux)
		errx(1, "%s: invalid reloc #%ld 0x%x",
		    ol->ol_path, r - os->os_rels, (unsigned)rel->r_info);
	if (rel->r_offset > shbits->sh_size)
		errx(1, "%s: reloc #%ld offset 0x%llx is out of range",
		    ol->ol_path, r - os->os_rels, (quad_t)rel->r_offset);

	sym = sidx[si];
	esym = &ELF_SYM(sym->sl_elfsym);

#if 0
	/*
	 * replace local compiler-generated symbols
	 * with section-relative and remove from the symtab
	 */
	snam = sym->sl_name;
	if (ELF_ST_BIND(esym->st_info) == STB_LOCAL && *snam &&
	    (Xflag == 2 || (*snam == 'L') ||
	     (snam[0] == '.' && snam[1] == 'L'))) {
		int j;
		for (j = 1; j < ol->ol_nsect; j++)
			if (ELF_ST_TYPE(ELF_SYM(sidx[j]->sl_elfsym).st_info) == STT_SECTION &&
			    ELF_SYM(sidx[j]->sl_elfsym).st_shndx == esym->st_shndx)
				break;
		if (j == ol->ol_nsect)
			errx(1, "%s: reloc for local symbol "
			    "\"%s\" without section %d",
			    ol->ol_path, snam, esym->st_shndx);
		addend += esym->st_value;
		sym_remove(sym);
		sidx[si = j] = sym = sidx[j];
	}
#endif

	r->rl_sym = sym;
	r->rl_addr = rel->r_offset;
	r->rl_addend = addend;
	r->rl_type = ELF_R_TYPE(rel->r_info);
	return 0;
}

/*
 * add another symbol from the object;
 * a hook called from elf_symload(3).
 * collect undefined symbols and resolve the defined
 */
int
elf_symadd(struct elf_symtab *es, int is, void *vs, void *v)
{
	static int next;	/* local symbols renaming */
	struct objlist *ol = v;
	struct section *os;
	Elf_Sym *esym = vs;
	struct symlist **sidx, *sym;
	char *name, *laname;

	if (!is && esym->st_shndx == SHN_UNDEF && esym->st_name == 0)
		return 0;

	/* allocate symindex */
	if (!ol->ol_aux) {
		if (!(ol->ol_aux = calloc(es->nsyms, sizeof sidx[0])))
			err(1, "calloc");
		ol->ol_naux = es->nsyms;
	}
	sidx = ol->ol_aux;

	/* skip file names and size defs */
	if (ELF_ST_TYPE(esym->st_info) == STT_FILE)
		return 0;

	if (esym->st_name >= es->stabsz)
		errx(1, "%s: invalid symtab entry #%d", es->name, is);
	name = es->stab + esym->st_name;

	/* convert all unwanted locals to sections */
	if (ELF_ST_BIND(esym->st_info) == STB_LOCAL &&
	    (Xflag == 2 || (Xflag == 1 && *name == 'L'))) {
		if (esym->st_shndx >= SHN_LORESERVE)
			errx(1, "unexpected section number 0x%x for %s",
			    esym->st_shndx, name);
		esym->st_info = ELF_ST_INFO(STB_LOCAL, STT_SECTION);
		esym->st_name = 0;
		name = es->stab;
	}

	if (ELF_ST_TYPE(esym->st_info) == STT_SECTION) {
		if (esym->st_shndx >= ol->ol_nsect)
			errx(1, "%s: corrupt symbol table", es->name);

		if (!(sym = calloc(1, sizeof *sym)))
			err(1, "calloc");

		ELF_SYM(sym->sl_elfsym) = *esym;
		sym->sl_sect = ol->ol_sections + esym->st_shndx;
		sym->sl_name = NULL;
		sidx[is] = sym;
		return 0;
	}

	laname = NULL;
if (is && *name == '\0') {
#define STT_GNU_SPARC_REGISTER STT_LOPROC
	if (ELF_ST_TYPE(esym->st_info) == STT_GNU_SPARC_REGISTER)
		errx(1, "rebuild %s with -mno-app-regs", ol->ol_name);
	else
		warnx("#%d is null", is);
}

	switch (esym->st_shndx) {
	case SHN_UNDEF:
		if (!(sym = sym_isdefined(name, sysobj.ol_sections)) &&
		    !(sym = sym_isundef(name))) {
			/* weak undef is a weak abs with NULL addr */
			if (ELF_ST_BIND(esym->st_info) == STB_WEAK)
				sym = elf_absadd(name, STB_WEAK);
			else {
				sym = sym_undef(name);
				sym->sl_sect = ol->ol_sections;
			}
		}
		break;

	case SHN_COMMON:
		if ((sym = sym_isdefined(name, ol->ol_sections))) {
			Elf_Sym *dsym = &ELF_SYM(sym->sl_elfsym);

			if (warncomm)
				warnx("%s: resolving common %s\n",
				    es->name, name);

			if (esym->st_size != dsym->st_size)
				warnx("%s: different size (%lu != %lu)", name,
				    (u_long)esym->st_size,
				    (u_long)dsym->st_size);
		} else if ((sym = sym_isundef(name)))
			sym = sym_define(sym, ol->ol_sections, vs);
		else
			sym = sym_add(name, ol->ol_sections, vs);
		break;

	case SHN_ABS:
		if (ELF_ST_BIND(esym->st_info) == STB_LOCAL) {
			if (asprintf(&name, "%s.%d", name, next++) < 0)
				err(1, "asprintf");
/* XXX leaking */
		}
		if ((sym = sym_isdefined(name, ol->ol_sections)))
			warnx("%s: %s: already defined in %s",
			    ol->ol_path, name,
			    sym->sl_sect->os_obj->ol_path);
		else if ((sym = sym_isundef(name)))
			sym = sym_define(sym, NULL, vs);
		else
			sym = sym_add(name, NULL, vs);
		break;

	default:
		if (esym->st_shndx >= ELF_HDR(ol->ol_hdr).e_shnum)
			err(1, "%s: invalid section index for #%d",
			    es->name, is);
		os = ol->ol_sections + esym->st_shndx;
		if ((sym = sym_isdefined(name, ol->ol_sections))) {
			if (ELF_ST_BIND(esym->st_info) == STB_LOCAL) {
				laname = NULL;
				do {
					if (sym->sl_next == INT_MAX)
						errx(1, "static overflow");
					if (asprintf(&name, "%s.%ld", name,
					    sym->sl_next++) < 0)
						err(1, "asprintf");
					if (laname)
						free(laname);
					laname = name;
					sym = sym_isdefined(name,
					    ol->ol_sections);
				} while (sym);
			} else if (ELF_ST_BIND(ELF_SYM(sym->sl_elfsym).st_info)
			    == STB_LOCAL) {
				char *nn;
				if (asprintf(&nn, "%s.%ld", name,
				    sym->sl_next++) < 0)
					err(1, "asprintf");
				/* discard const */
				/*
				 * here we assume that the new name would
				 * not change the symbol tree order
				 * (since names like this did not exist)
				 */
				laname = (char *)sym->sl_name;
				sym->sl_name = nn;
				sym = NULL;
			}
		}

		if (sym &&
		    ELF_ST_BIND(ELF_SYM(sym->sl_elfsym).st_info) == STB_WEAK)
			sym = sym_redef(sym, os, esym);
		else if (sym && ELF_SYM(sym->sl_elfsym).st_shndx == SHN_COMMON)
			sym = sym_redef(sym, os, esym);
		else if (sym)
			warnx("%s: %s: already defined in %s",
			    ol->ol_path, name,
			    sym->sl_sect->os_obj->ol_path);
		else if ((sym = sym_isundef(name)))
			sym = sym_define(sym, os, vs);
		else
			sym = sym_add(name, os, vs);
		if (laname)
			free(laname);
	}

	sidx[is] = sym;
	if (cref && sym && ol) {
		struct xreflist *xl;

		if (!(xl = calloc(1, sizeof *xl)))
			errx(1, "calloc");
		xl->xl_obj = ol;
		if (sym->sl_sect && sym->sl_sect->os_obj == ol)
			TAILQ_INSERT_HEAD(&sym->sl_xref, xl, xl_entry);
		else
			TAILQ_INSERT_TAIL(&sym->sl_xref, xl, xl_entry);
	}
	return 0;
}

/*
 * add an object to the link editing ordee;
 * load needed headers and relocations where available;
 * load symbols and resolve references from relocs and other
 * objects (as undefined);
 * prepare the relocs for the final output and sort by address.
 */
int
elf_objadd(struct objlist *ol, FILE *fp, off_t foff)
{
	struct section *os;
	struct elf_symtab es;
	Elf_Ehdr *eh;
	Elf_Shdr *shdr;
	int i, n;

	eh = &ELF_HDR(ol->ol_hdr);
	if (elf_ld_chkhdr(ol->ol_name, eh, ET_REL,
	    &machine, &elfclass, &endian))
		return 1;

	if (!(shdr = elf_load_shdrs(ol->ol_name, fp, foff, eh)))
		return 1;
	elf_fix_shdrs(eh, shdr);

	n = ol->ol_nsect = eh->e_shnum;
	if (!(ol->ol_sections = calloc(n, sizeof(struct section))))
		err(1, "calloc");

	for (i = 0, os = ol->ol_sections; i < n; os++, i++) {
		os->os_no = i;
		os->os_sect = &shdr[i];
		os->os_obj = ol;
		os->os_off = foff + shdr[i].sh_offset;
		os->os_flags = shdr[i].sh_flags;
		TAILQ_INIT(&os->os_syms);
	}

	/* load symbol table */
	es.name = ol->ol_name;
	es.ehdr = eh;
	es.shdr = shdr;
	es.shstr = NULL;

	if (elf_symload(&es, fp, foff, elf_symadd, ol))
		return 1;
	ol->ol_sects = es.shdr;
	ol->ol_snames = es.shstr;
	free(es.stab);

	/* scan thru the section list looking for progbits and relocs */
	for (i = 0, os = ol->ol_sections; i < n; shdr++, os++, i++) {
		os->os_name = ol->ol_snames + shdr->sh_name;
		if (shdr->sh_type == SHT_NULL)
			continue;

		if (shdr->sh_type == SHT_NOBITS) {
			if (!strcmp(os->os_name, ELF_BSS)) {
				if (ol->ol_bss) {
					warnx("%s: too many .bss sections",
					    ol->ol_name);
					errors++;
				} else
					ol->ol_bss = os;
			}
			continue;
		}

		if (shdr->sh_type != SHT_PROGBITS)
			continue;

		if (i + 1 >= n)
			continue;

		if (shdr[1].sh_type != SHT_RELA &&
		    shdr[1].sh_type != SHT_REL)
			continue;

		if (elf_loadrelocs(ol, os, &shdr[1], fp, foff))
			continue;
	}

	free(ol->ol_aux);
	ol->ol_aux = NULL;

	return 0;
}

/*
 * check if object is editable by linking;
 * first object on the command line defines the machine
 * and endianess whilst later ones have to match.
 */
int
elf_ld_chkhdr(const char *path, Elf_Ehdr *eh, int type, int *mach,
    int *class, int *endian)
{

	if (eh->e_type != type) {
		warnx("%s: not a relocatable file", path);
		return EFTYPE;
	}

	if (*mach == EM_NONE) {
		*mach = eh->e_machine;
		*class = eh->e_ident[EI_CLASS];
		*endian = eh->e_ident[EI_DATA];
		ldarch = ldinit();
		return ldarch == NULL? EINVAL : 0;
	}

	if (*mach != eh->e_machine ||
	    *class != eh->e_ident[EI_CLASS] ||
	    *endian != eh->e_ident[EI_DATA]) {
		warnx("%s: incompatible file type", path);
		return EFTYPE;
	}

	return 0;
}

/*
 * seek forward in the output file and fill
 * the gap with the filler (not yet ;)
 */
int
elf_seek(FILE *fp, off_t off, uint64_t filler)
{
	if (fseeko(fp, off, SEEK_SET) < 0)
		return -1;

	return 0;
}

/*
 * a micro-linker that is only used for cleaning up
 * a single file from unwanted symbol entries and
 * thus simplify relocs and shrink symbol table;
 * depending on the flags may as well completely
 * fix the object in memory thus performing "dynamic" loading
 *
 * operates on the file already in-memory
 */
int
uLD(const char *name, char *v, int *size, int flags)
{
	Elf_Ehdr *eh = (Elf_Ehdr *)v;
	Elf_Shdr *shdr, *sh, *esh, *ssh;
	Elf_Sym **syms;
	char *strs, *estr, *p;
	int i, j, k, nsyms, *nus, *sect;

	shdr = (Elf_Shdr *)(v + eh->e_shoff);
	/* find the symbol table */
	for (sh = shdr, esh = sh + eh->e_shnum; sh < esh; sh++)
		if (sh->sh_type == SHT_SYMTAB)
			break;
	if (sh >= esh)
		errx(1, "%s: no symbol table", name);

	if (sh->sh_link >= eh->e_shnum)
		errx(1, "%s: invalid symtab link", name);

	esh = shdr + sh->sh_link;
	if (esh->sh_type != SHT_STRTAB)
		errx(1, "%s: no strings attached", name);

	if (sh->sh_offset >= *size || sh->sh_offset + sh->sh_size > *size)
		errx(1, "%s: corrupt section header #%td", name, sh - shdr);

	if (esh->sh_offset >= *size || esh->sh_offset + esh->sh_size > *size)
		errx(1, "%s: corrupt section header #%td", name, esh - shdr);

	if (sh->sh_size / sh->sh_entsize >= INT_MAX)
		errx(1, "%s: symtab is too big %llu",
		    name, (long long)sh->sh_size);

	if (sh->sh_entsize < sizeof **syms)
		errx(1, "%s: invalid symtab entry size", name);

	ssh = sh;
	nsyms = sh->sh_size / sh->sh_entsize;
	if (!(syms = calloc(nsyms, sizeof *syms)))
		err(1, "calloc");

	if (!(nus = calloc(nsyms, sizeof *nus)))
		err(1, "calloc");

	if (!(sect = calloc(eh->e_shnum, sizeof *sect)))
		err(1, "calloc");

	for (i = 0; i < nsyms; i++) {
		syms[i] = (Elf_Sym *)(v + sh->sh_offset + i * sh->sh_entsize);
		syms[i]->st_other = 1;
		nus[i] = i;
		if (ELF_ST_TYPE(syms[i]->st_info) == STT_SECTION) {
			/* XXX for now do not care about MD sections */
			if (syms[i]->st_shndx >= eh->e_shnum)
				errx(1, "%s: invalid shndx in a sym #%d",
				    name, i);
			sect[syms[i]->st_shndx] = i;
		}
	}
	strs = v + esh->sh_offset;
	estr = strs + esh->sh_size;
	if (estr[-1] != '\0')
		errx(1, "%s: unterminated strings section", name);

	/* fix relocs; mark symbols to dispose */
	for (sh = shdr, esh = sh + eh->e_shnum; sh < esh; sh++) {
		Elf_Shdr *rsh;
		int nrels;

		if ((rsh = sh + 1) >= esh)
			continue;

		if (rsh->sh_type != SHT_REL && rsh->sh_type != SHT_RELA)
			continue;

		if (rsh->sh_offset >= *size ||
		    rsh->sh_offset + rsh->sh_size > *size)
			errx(1, "%s: corrupt section header #%td",
			    name, rsh - shdr);

		if ((rsh->sh_type == SHT_REL &&
		     rsh->sh_entsize < sizeof(Elf_Rel)) ||
		    (rsh->sh_type == SHT_RELA &&
		     rsh->sh_entsize < sizeof(Elf_RelA)))
			errx(1, "%s: invalid reloc entry size", name);

		if (rsh->sh_size / rsh->sh_entsize >= INT_MAX)
			errx(1, "%s: too many relocs for section %td",
			    name, sh - shdr);

		/* take a point of the relocs */
		p = v + rsh->sh_offset;
		nrels = rsh->sh_size / rsh->sh_entsize;
		for (i = 0; i < nrels; i++) {
			Elf_Rel *r = (Elf_Rel *)(p + i * rsh->sh_entsize);
			Elf_RelA *ra = (Elf_RelA *)(p + i * rsh->sh_entsize);
			Elf_Sym *sym;
			char *snam;

			/* XXX this does not include the reloc size */
			if (r->r_offset >= sh->sh_size)
				errx(1, "%s: reloc #%d offset out of range",
				     name, i);

			if (ELF_R_SYM(r->r_info) >= nsyms)
				errx(1, "%s: broken reloc #%d for section %td",
				    name, i, sh - shdr);

			sym = syms[ELF_R_SYM(r->r_info)];
			snam = strs + sym->st_name;
			if (snam >= estr)
				errx(1, "%s: corrupt syment", name);
			
			/* skip externals */
			if (sym->st_shndx == SHN_UNDEF)
				continue;

			/* skip well [un]defined symbols */
			if (!sym->st_shndx || sym->st_shndx >= eh->e_shnum)
				continue;

			/* skip over non-local symbols */
			if (ELF_ST_BIND(sym->st_info) != STB_LOCAL ||
			    !((snam[0] == '.' && snam[1] == 'L') ||
			    *snam == 'L' || flags > 1)) {
				sym->st_other = 0;
				continue;
			}

			/*
			 * move over local syms's relocs to become
			 * section-relative;
			 * relAs are easy -- just add to the addendum;
			 * plain rels oughtta sought MD knowledge
			 */
			if (rsh->sh_type == SHT_RELA)
				ra += sym->st_value;
			else switch (eh->e_machine) {
#if ELFSIZE == 32
			case EM_ARM:
				arm_fixone(v + sh->sh_offset + r->r_offset,
				    sym->st_value, 0, ELF_R_TYPE(r->r_info));
				break;
			case EM_386:
				i386_fixone(v + sh->sh_offset + r->r_offset,
				    sym->st_value, 0, ELF_R_TYPE(r->r_info));
				break;
#else /* ELFSIZE == 64 */
			case EM_AMD64:
				amd64_fixone(v + sh->sh_offset + r->r_offset,
				    sym->st_value, 0, ELF_R_TYPE(r->r_info));
				break;
			case EM_SPARCV9:
				sparc64_fixone(v + sh->sh_offset + r->r_offset,
				    sym->st_value, 0, ELF_R_TYPE(r->r_info));
				break;
#endif
			case EM_PARISC:
				hppa_fixone(v + sh->sh_offset + r->r_offset,
				    sym->st_value, 0, ELF_R_TYPE(r->r_info));
				break;
			}

			if (!sect[sym->st_shndx])
				errx(1, "%s: missing section #%d",
				    name, sym->st_shndx);
			nus[ELF_R_SYM(r->r_info)] = sect[sym->st_shndx];
		}
	}

	/* recycle syms; minimise calls to memcpy */
	for (i = j = k = 0; i < nsyms; i++) {
		Elf_Sym *sym;
		char *snam;

		sym = syms[i];
		if (!sym->st_other) {
			if (!k)
				k = i;
			continue;
		}

		/* skip over non-local symbols */
		snam = strs + sym->st_name;
		if (ELF_ST_BIND(sym->st_info) != STB_LOCAL ||
		    !((snam[0] == '.' && snam[1] == 'L') ||
		    *snam == 'L' || flags > 1)) {
			sym->st_other = 0;
			if (!k)
				k = i;
			continue;
		}

		/* mark the beginning of the free fragment */
		if (j == 0) {
			j = i;
			k = 0;
			continue;
		}

		/* still running through the locals */
		if (!k)
			continue;

/*
printf("hit %s i %d j %d k %d\n", snam, i, j, k);
*/
		memcpy(syms[j], syms[k], (i - k) * ssh->sh_entsize);
		/* mark up new positions for the moved syms */
		for (k = j + i - k; j < k; j++)
			nus[i - (k - j)] = j;
		k = 0;
	}

	/* extra step in case we had bunch of 'em before the end */
	if (k && k < i) {
/*
printf("i %d j %d k %d\n", i, j, k);
*/
		memcpy(syms[j], syms[k], (i - k) * ssh->sh_entsize);
		for (k = j + i - k; j < k; j++)
			nus[i - (k - j)] = j;
	}
	nsyms = j;

	/* run through the relocs again and map symbol refs */
	for (sh = shdr, esh = sh + eh->e_shnum; sh < esh; sh++) {
		Elf_Shdr *rsh;
		int nrels;

		if ((rsh = sh + 1) >= esh)
			continue;

		if (rsh->sh_type != SHT_REL && rsh->sh_type != SHT_RELA)
			continue;

		p = v + rsh->sh_offset;
		nrels = rsh->sh_size / rsh->sh_entsize;
		for (i = 0; i < nrels; i++) {
			Elf_Rel *r = (Elf_Rel *)(p + i * rsh->sh_entsize);
			int ns = ELF_R_SYM(r->r_info);

/*
printf("ns %d -> %d\n", ns, nus[ns]);
*/
			r->r_info = ELF_R_INFO(nus[ns], ELF_R_TYPE(r->r_info));
		}
	}

	if (!flags)
		return 0;

	/* adjust .symtab size */
	j = nsyms * ssh->sh_entsize;
	k = ssh->sh_size - j;
	ssh->sh_size = j;
	sh = shdr + ssh->sh_link;
	sh->sh_offset = ssh->sh_offset + ssh->sh_size;
	p = v + sh->sh_offset;
	*p++ = '\0';
	/* generate .strtab; we assume copy would not overlap (; */
	for (j = i = 1; i < nsyms; i++) {
		char *snam = strs + syms[i]->st_name;
		/* skip those we have already put out */
		if (syms[i]->st_name < j)
			continue;
		j = syms[i]->st_name;
		syms[i]->st_name = p - (v + sh->sh_offset);
		p += strlcpy(p, snam, sh->sh_size) + 1;
	}
	*p++ = '\0';
	j = p - (v + sh->sh_offset);
	k += sh->sh_size - j;
	sh->sh_size = j;
	/* offset all the following sections; assuming they are sorted */
	for (ssh = sh, sh = shdr, esh = sh + eh->e_shnum; sh < esh; sh++)
		if (sh->sh_offset > ssh->sh_offset) {
			sh->sh_offset -= k;
			memcpy(v + sh->sh_offset, v + sh->sh_offset + k,
			    sh->sh_size);
		}
	*size -= k;

	return 0;
}
