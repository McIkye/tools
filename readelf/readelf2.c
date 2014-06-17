/*
 * Copyright (c) 2012-2014 Michael Shalayeff
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

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <elf_abi.h>
#include <elfuncs.h>

#include "readelf.h"

#ifndef lint
static const char rcsid[] = "$ABSD: readelf2.c,v 1.4 2014/06/18 11:46:51 mickey Exp $";
#endif

#if ELFSIZE == 32
#define	ELF_HDR(h)	((h).elf32)
#define	ELF_SYM(h)	((h).sym32)
#define	elf_fix_header	elf32_fix_header
#define	elf_load_phdrs	elf32_load_phdrs
#define	elf_fix_phdrs	elf32_fix_phdrs
#define	elf_load_shdrs	elf32_load_shdrs
#define	elf_fix_shdrs	elf32_fix_shdrs
#define	elf_shstrload	elf32_shstrload
#define	elf_symload	elf32_symload
#define	elf_symadd	elf32_symadd
#define	elf_gethdrs	elf32_gethdrs
#define	elf_verify	elf32_verify
#define	elf_prhdr	elf32_prhdr
#define	elf_prsegs	elf32_prsegs
#define	elf_prsechs	elf32_prsechs
#define	elf_prsyms	elf32_prsyms
#define	elf_prels	elf32_prels
#define	elf_fix_rel	elf32_fix_rel
#define	elf_fix_rela	elf32_fix_rela
#elif ELFSIZE == 64
#define	ELF_HDR(h)	((h).elf64)
#define	ELF_SYM(h)	((h).sym64)
#define	elf_fix_header	elf64_fix_header
#define	elf_load_phdrs	elf64_load_phdrs
#define	elf_fix_phdrs	elf64_fix_phdrs
#define	elf_load_shdrs	elf64_load_shdrs
#define	elf_fix_shdrs	elf64_fix_shdrs
#define	elf_shstrload	elf64_shstrload
#define	elf_symload	elf64_symload
#define	elf_symadd	elf64_symadd
#define	elf_gethdrs	elf64_gethdrs
#define	elf_verify	elf64_verify
#define	elf_prhdr	elf64_prhdr
#define	elf_prsegs	elf64_prsegs
#define	elf_prsechs	elf64_prsechs
#define	elf_prsyms	elf64_prsyms
#define	elf_prels	elf64_prels
#define	elf_fix_rel	elf64_fix_rel
#define	elf_fix_rela	elf64_fix_rela
#else
#error "Unsupported ELF class"
#endif

int
elf_symadd(struct elf_symtab *es, int is, void *vs, void *v)
{
	Elf_Sym *esym = vs;

	if (!symidx && !(symidx = calloc(es->nsyms, sizeof symidx[0])))
		err(1, "calloc");

	if (esym->st_name >= es->stabsz)
		errx(1, "%s: invalid symtab entry #%d", es->name, is);

	if (ELF_ST_TYPE(esym->st_info) == STT_SECTION)
		symidx[is].sl_name = es->shstr +
		    ((Elf_Shdr *)(es->shdr + esym->st_shndx *
		    ((Elf_Ehdr *)es->ehdr)->e_shentsize))->sh_name;
	else
		symidx[is].sl_name = es->stab + esym->st_name;
	ELF_SYM(symidx[is].sl_elfsym) = *esym;

	return 0;
}

int
elf_gethdrs(FILE *fp, const char *name, off_t off, Elf_Ehdr *eh,
    void **p, void **s, void **sn)
{
	elf_fix_header(eh);

	if (eh->e_ehsize < sizeof(Elf_Ehdr) ||
	    (eh->e_phentsize && eh->e_phentsize < sizeof(Elf_Phdr)) ||
	    eh->e_shentsize < sizeof(Elf_Shdr) ||
	    eh->e_shstrndx > eh->e_shnum)
		return -1;

	if (!(*p = elf_load_phdrs(name, fp, off, eh)))
		return -1;
	elf_fix_phdrs(eh, *p);

	if (!(*s = elf_load_shdrs(name, fp, off, eh)))
		return -1;
	elf_fix_shdrs(eh, *s);

	if (!(*sn = elf_shstrload(name, fp, off, eh, *s)))
		return -1;

	if (opts & (SYMS | RELOCS | VERIFY)) {
		struct elf_symtab es;

		es.name = name;
		es.ehdr = eh;
		es.shdr = *s;
		es.shstr = *sn;
		if (elf_symload(&es, fp, off, elf_symadd, NULL))
			return -1;
		nsyms = es.nsyms;
	}

	return 0;
}

int
elf_verify(Elf_Ehdr *eh, void *vp, void *vs, const char *sn)
{

	return 0;
}

void
elf_prhdr(Elf_Ehdr *eh)
{
	int i;

	printf("ELF Header:\n  Magic:  ");

	for (i = 0; i < EI_NIDENT; i++)
		printf(" %02x", eh->e_ident[i]);

	printf("\n");

	printf("  %-34s ELF%d\n", "Class:",
	    eh->e_ident[EI_CLASS] == ELFCLASS32? 32 : 64);

	printf("  %-34s %s endian\n", "Data:",
	    eh->e_ident[EI_DATA] == ELFDATA2LSB? "little" : "big");

	printf("  %-34s %u\n", "Version:", eh->e_ident[EI_VERSION]);

	printf("  %-34s %s\n", "OS/ABI:", elf_osabi(eh->e_ident[EI_OSABI]));

	printf("  %-34s %u\n", "ABI Version:", eh->e_ident[EI_ABIVERSION]);

	printf("  %-34s %s\n", "Type:", elf_type(eh->e_type));

	printf("  %-34s %s\n", "Machine:", elf_machine(eh->e_machine));

	printf("  %-34s %u\n", "Version:", (int)eh->e_version);

	printf("  %-34s 0x%llx\n", "Entry point address:",
	    (uint64_t)eh->e_entry);

	printf("  %-34s %llu\n", "Start of program headers:",
	    (uint64_t)eh->e_phoff);

	printf("  %-34s %llu\n", "Start of section headers:",
	    (uint64_t)eh->e_shoff);

	printf("  %-34s 0x%x\n", "Flags:", (uint32_t)eh->e_flags);

	printf("  %-34s %u\n", "Size of this header:", (int)eh->e_ehsize);

	printf("  %-34s %u\n", "Size of program headers:",
	    (int)eh->e_phentsize);
	
	printf("  %-34s %u\n", "Number of program headers:", (int)eh->e_phnum);
	
	printf("  %-34s %u\n", "Size of section headers:",
	    (int)eh->e_shentsize);
	
	printf("  %-34s %u\n", "Number of section headers:", (int)eh->e_shnum);
	
	printf("  %-34s %u\n", "Section header string table index:",
	    (int)eh->e_shstrndx);

/* TODO dump extra bytes */
}

void
elf_prsegs(Elf_Ehdr *eh, void *v)
{
	int i;

	if (!(opts & FILEH)) {
		printf("Elf file type is %s\n", elf_type(eh->e_type));
		printf("Entry point is 0x%llx\n", (uint64_t)eh->e_entry);
		printf("There are %u program headers, "
		    "starting at offset %llu\n",
		    (int)eh->e_phnum, (uint64_t)eh->e_phoff);
	}

	printf("\nProgram Headers:\n");

	printf("  %-14s %-8s %-10s %-10s %-7s %-7s %-3s %-s\n",
	    "Type", "Offset", "VirtAddr", "PhysAddr", "FileSiz",
	    "MemSiz", "Flg", "Align");
	for (i = 0; i < eh->e_phnum; i++) {
		Elf_Phdr *ph = v + i * eh->e_phentsize;

		printf("  %-14s 0x%06llx 0x%08llx 0x%08llx 0x%05llx 0x%05llx "
		    "%3s 0x%x\n",
		    elf_phtype(ph->p_type), (off_t)ph->p_offset,
		    (uint64_t)ph->p_vaddr, (uint64_t)ph->p_paddr,
		    (off_t)ph->p_filesz, (off_t)ph->p_memsz,
		    elf_pflags(ph->p_flags), (int)ph->p_align);
	}

/* TODO segment to section mapping */
}

void
elf_prsechs(Elf_Ehdr *eh, void *v, const char *n)
{
	int i;

	if (!(opts & FILEH)) {
		printf("There are %u section headers, "
		    "starting at offset 0x%llx:\n",
		    (int)eh->e_shnum, (uint64_t)eh->e_shoff);
	}

	printf("\nSection Headers:\n");
	printf("  [Nr] %-18s %-14s %-8s %-6s %-6s %2s %3s %2s %3s %2s\n",
	    "Name", "Type", "Addr", "Off", "Size", "ES", "Flg", "Lk", "Inf",
	    "Al");
	for (i = 0; i < eh->e_shnum; i++) {
		Elf_Shdr *sh = v + i * eh->e_shentsize;

		printf("  [%02d] %-18s %-14s %08llx %06llx %06llx %2s %3s %2d %2d %2d\n",
		    i, n + sh->sh_name, elf_shtype(sh->sh_type),
		    (uint64_t)sh->sh_addr, (uint64_t)sh->sh_offset,
		    (uint64_t)sh->sh_size,
		    "00", elf_sflags(sh->sh_flags), sh->sh_link, sh->sh_info,
		    (int)sh->sh_addralign);
	}
}

void
elf_prsyms(Elf_Ehdr *eh, void *v, const char *sn)
{
	struct sym *s, *es;
	Elf_Shdr *sh;
	Elf_Sym *sym;
	int i;

/* TODO only works for one symtab XXX */
	for (i = 0, sh = v; i < eh->e_shnum; sh = v + ++i * eh->e_shentsize)
		if (sh->sh_type == SHT_SYMTAB || sh->sh_type == SHT_DYNSYM) {

			printf("\nSymbol table '%s' contains %ld entries:\n",
			    sn + sh->sh_name,
			    (long)(sh->sh_size / sh->sh_entsize));

			printf("%6s: %8s %5s %-7s %-6s %-8s %3s %s\n",
			    "Num", "Value", "Size", "Type", "Bind", "Vis",
			    "Ndx", "Name");

			for (s = symidx, es = s + nsyms; s < es; s++) {
				sym = &ELF_SYM(s->sl_elfsym);
				printf("%6ld: %08lx %5lu %-7s %-6s %-8s %3s %s\n",
				    s - symidx, (long)sym->st_value,
				    (u_long)sym->st_size,
				    elf_symtype(ELF_ST_TYPE(sym->st_info)),
				    elf_symbind(ELF_ST_BIND(sym->st_info)),
				    "DEFAULT", elf_shndx(sym->st_shndx),
				    s->sl_name);
			}
		}
}

void
elf_prels(FILE *fp, const char *name, off_t off,
    Elf_Ehdr *eh, void *v, const char *sn)
{
	Elf_Shdr *sh;
	int i;

	for (i = 0, sh = v; i < eh->e_shnum; sh = v + ++i * eh->e_shentsize) {
		int n;

		if (sh->sh_type != SHT_REL && sh->sh_type != SHT_RELA)
			continue;

		if (fseeko(fp, off + sh->sh_offset, SEEK_SET))
			err(1, "fseeko: %s", name);

		n = sh->sh_size / sh->sh_entsize;
		printf("\nRelocation section \'%s\' at offset 0x%lx "
		    "contains %d entries:\n", sn + sh->sh_name,
		    (long)sh->sh_offset, n);
		printf("%-8s %-8s %-16s  %9s   %s\n", " Offset",
		    "  Info", " Type", "Sym.Value",
		    sh->sh_type == SHT_REL? "Sym. Name" : "Sym. Name + Addend");

		if (sh->sh_type == SHT_REL) {
			Elf_Rel r;
			int j;

			if (sh->sh_entsize < sizeof r ||
			    sh->sh_size % sh->sh_entsize) {
				warnx("%s: invalid REL section #%d", name, i);
				break;
			}

			for (j = 0; j < n; j++) {
				struct sym *sym;
				int si;

				if (fread(&r, sizeof r, 1, fp) != 1)
					err(1, "fread: %s", name);
				elf_fix_rel(eh, &r);
#if ELFSIZE == 64
				if (eh->e_machine == EM_MIPS)
					si = ELF64_R_MIPS_SYM(r.r_info);
				else
#endif
				si = ELF_R_SYM(r.r_info);
				if (si >= nsyms)
					errx(1, "%s: invalid REL #%d",
					    name, j);
				sym = &symidx[si];
				printf("%08llx %08x %-16s  %08llx   %s\n",
				    (uint64_t)r.r_offset, (uint32_t)r.r_info,
				    elf_reltype(eh->e_machine,
#if ELFSIZE == 64
				    eh->e_machine == EM_MIPS?
				      betoh32(ELF64_R_MIPS_TYPE(r.r_info)) :
#endif
				      ELF_R_TYPE(r.r_info)),
				    (uint64_t)ELF_SYM(sym->sl_elfsym).st_value, 
				    sym->sl_name);
/* TODO deal with "complex" mips relocs */
			}
		} else {
			Elf_RelA r;
			int j;

			if (sh->sh_entsize < sizeof r ||
			    sh->sh_size % sh->sh_entsize) {
				warnx("%s: invalid RELA section #%d", name, i);
				break;
			}

			for (j = 0; j < n; j++) {
				struct sym *sym;
				int si;

				if (fread(&r, sizeof r, 1, fp) != 1)
					err(1, "fread: %s", name);
				elf_fix_rela(eh, &r);
#if ELFSIZE == 64
				if (eh->e_machine == EM_MIPS)
					si = ELF64_R_MIPS_SYM(r.r_info);
				else
#endif
				si = ELF_R_SYM(r.r_info);
				if (si >= nsyms)
					errx(1, "%s: invalid RELA #%d",
					    name, j);
				sym = &symidx[si];
				printf("%08llx %08x %-16s  %08llx   %s",
				    (uint64_t)r.r_offset, (uint32_t)r.r_info,
				    elf_reltype(eh->e_machine,
#if ELFSIZE == 64
				    eh->e_machine == EM_MIPS?
				      betoh32(ELF64_R_MIPS_TYPE(r.r_info)) :
#endif
				      ELF_R_TYPE(r.r_info)),
				    (uint64_t)ELF_SYM(sym->sl_elfsym).st_value, 
				    sym->sl_name);
				if (r.r_addend)
					printf(" + %llx",
					    (long long)r.r_addend);
/* TODO deal with "complex" mips relocs */
				putchar('\n');
			}
		}
	}
}
