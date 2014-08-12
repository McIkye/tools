/*
 * Copyright (c) 2009-2010 Michael Shalayeff
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

#ifndef _LIBELF_H_
#define _LIBELF_H_

#define	ELFLIB_STRIPX	0x01
#define	ELFLIB_STRIPD	0x02

struct nlist;
struct elf_symtab {
		/* from the caller */
	const char *name;	/* objname */
	const void *ehdr;	/* file header */

		/* if empty we we will provide */
	void *shdr;		/* sections headers */
	char *shstr;		/* sections header strings */

		/* we provide these */
	char	*stab;		/* strings table for the syms */
	size_t	stabsz;		/* strings size */
	u_long	nsyms;		/* number of symbols in the table */
};

/* flags for elf_dwarfnebula */
#define	ELF_DWARF_ADDRS	0x01
#define	ELF_DWARF_LINES	0x02
#define	ELF_DWARF_NAMES	0x04
#define	ELF_DWARF_TYPES	0x08

struct dwarf_line {
	uint64_t addr;		/* start address for */
	ssize_t len;		/* length of the compilation unit */
	const uint8_t *lnp;	/* line number program */
};

struct dwarf_name {
	uint64_t addr;		/* always store in 64bits as we do not know */
	ssize_t len;		/* length of the object */
	const char *name;	/* symbol name */
	const char *unit;	/* ptr into corresponding compile unit */
};

struct dwarf_nebula {
	const char *name;	/* objname */
	const uint8_t *unit;	/* current compilation unit in iterators */
	int is64;		/* DWARF size 4/8 */
	int a64;		/* address length 4/8 */

	const uint8_t *abbrv;	/* .debug_addrev */
	ssize_t	nabbrv;		/* size of the abbreviations section */
	const uint8_t *info;	/* .debug_info */
	ssize_t	ninfo;		/* size of the debugging info */
	ssize_t	nunits;		/* number of compile units in the info */

	const uint8_t *lines;	/* .debug_lines */
	ssize_t	nlines;		/* size of the line numbers info */
	struct dwarf_line *a2l;	/* addr-sorted array of line infos */
/* TODO create a file:line indexed list */

	const uint8_t *names;	/* .debug_pubnames */
	ssize_t	nnames;		/* size of the pub names info */
	struct dwarf_name *a2n;	/* address-sorted list */
	struct dwarf_name *n2a;	/* name-sorted list */
	ssize_t ncount;		/* number of entries in the index */

	const uint8_t *str;	/* .debug_str */
	ssize_t	nstr;		/* size of the strings info */

	/* misc */
	unsigned char elfdata;	/* cached EI_DATA */
};

int	elf_checkoff(const char *, FILE *, off_t, off_t);

int	elf32_fix_header(Elf32_Ehdr *eh);
int	elf32_chk_header(Elf32_Ehdr *eh);
int	elf32_fix_note(Elf32_Ehdr *, Elf32_Note *);
Elf32_Shdr*elf32_load_shdrs(const char *, FILE *, off_t, const Elf32_Ehdr *);
Elf32_Shdr*elf32_scan_shdrs(const Elf32_Ehdr *, Elf32_Shdr *, const char *,
	    int (*)(Elf32_Shdr *, const char *, void *), void *);
int	elf32_save_shdrs(const char *, FILE *, off_t, const Elf32_Ehdr *,
	    const Elf32_Shdr *);
int	elf32_save_phdrs(const char *, FILE *, off_t, const Elf32_Ehdr *,
	    const Elf32_Phdr *);
Elf32_Phdr*elf32_load_phdrs(const char *, FILE *, off_t, const Elf32_Ehdr *);
Elf32_Phdr*elf32_scan_phdrs(const Elf32_Ehdr *, Elf32_Phdr *,
	    int (*)(Elf32_Phdr *, void *), void *);
int	elf32_fix_shdrs(const Elf32_Ehdr *eh, Elf32_Shdr *shdr);
int	elf32_fix_phdrs(const Elf32_Ehdr *eh, Elf32_Phdr *phdr);
int	elf32_fix_rel(Elf32_Ehdr *, Elf32_Rel *);
int	elf32_fix_rela(Elf32_Ehdr *, Elf32_Rela *);
int	elf32_fix_sym(const Elf32_Ehdr *eh, Elf32_Sym *sym);
int	elf32_2nlist(Elf32_Sym *, const Elf32_Ehdr *, const Elf32_Shdr *,
	    const char *, struct nlist *);
int	elf32_size(const Elf32_Ehdr *, Elf32_Shdr *,
	    u_long *, u_long *, u_long *);
char	*elf32_sld(const char *, FILE *, off_t, const Elf32_Shdr *shdr);
char	*elf32_shstrload(const char *, FILE *, off_t, const Elf32_Ehdr *,
	    const Elf32_Shdr *shdr);
int	elf32_symload(struct elf_symtab *, FILE *, off_t,
	    int (*func)(struct elf_symtab *, int, void *, void *), void *arg);

int	elf64_fix_header(Elf64_Ehdr *eh);
int	elf64_chk_header(Elf64_Ehdr *eh);
int	elf64_fix_note(Elf64_Ehdr *, Elf64_Note *);
Elf64_Shdr*elf64_load_shdrs(const char *, FILE *, off_t, const Elf64_Ehdr *);
Elf64_Shdr*elf64_scan_shdrs(const Elf64_Ehdr *, Elf64_Shdr *, const char *,
	    int (*)(Elf64_Shdr *, const char *, void *), void *);
int	elf64_save_shdrs(const char *, FILE *, off_t, const Elf64_Ehdr *,
	    const Elf64_Shdr *);
int	elf64_save_phdrs(const char *, FILE *, off_t, const Elf64_Ehdr *,
	    const Elf64_Phdr *);
Elf64_Phdr*elf64_load_phdrs(const char *, FILE *, off_t, const Elf64_Ehdr *);
Elf64_Phdr*elf64_scan_phdrs(const Elf64_Ehdr *, Elf64_Phdr *,
	    int (*)(Elf64_Phdr *, void *), void *);
int	elf64_fix_shdrs(const Elf64_Ehdr *eh, Elf64_Shdr *shdr);
int	elf64_fix_phdrs(const Elf64_Ehdr *eh, Elf64_Phdr *phdr);
int	elf64_fix_sym(const Elf64_Ehdr *eh, Elf64_Sym *sym);
int	elf64_fix_rel(Elf64_Ehdr *, Elf64_Rel *);
int	elf64_fix_rela(Elf64_Ehdr *, Elf64_Rela *);
int	elf64_2nlist(Elf64_Sym *, const Elf64_Ehdr *, const Elf64_Shdr *,
	    const char *, struct nlist *);
int	elf64_size(const Elf64_Ehdr *, Elf64_Shdr *,
	    u_long *, u_long *, u_long *);
char	*elf64_sld(const char *, FILE *, off_t, const Elf64_Shdr *shdr);
char	*elf64_shstrload(const char *, FILE *, off_t, const Elf64_Ehdr *,
	    const Elf64_Shdr *shdr);
int	elf64_symload(struct elf_symtab *, FILE *, off_t,
	    int (*func)(struct elf_symtab *, int, void *, void *), void *arg);

struct dwarf_nebula *
	elf32_dwarfnebula(const char*, FILE *, off_t, const Elf32_Ehdr*, int);
struct dwarf_nebula *
	elf64_dwarfnebula(const char*, FILE *, off_t, const Elf64_Ehdr*, int);

uint64_t dwarf_off48(struct dwarf_nebula *, const uint8_t **);
int dwarf_ilen(struct dwarf_nebula*,const uint8_t**,ssize_t*,uint64_t*,int*);
int	dwarf_leb128(uint64_t *, const uint8_t **, ssize_t *, int);

ssize_t	dwarf_info_count(struct dwarf_nebula *);
int	dwarf_info_abbrv(struct dwarf_nebula *, const uint8_t **, ssize_t *,
	    ssize_t *, ssize_t *);
int	dwarf_info_scan(struct dwarf_nebula *,
	    int (*)(struct dwarf_nebula *, const uint8_t *, ssize_t, void *,
	    ssize_t), void *);
int	dwarf_info_lines(struct dwarf_nebula *);

int	dwarf_abbrv_find(struct dwarf_nebula*, const uint8_t**, ssize_t*,int);
int	dwarf_attr(struct dwarf_nebula *, const uint8_t **, ssize_t *,
	    void *, ssize_t *, int);

int	dwarf_names_scan(struct dwarf_nebula *,
	    int (*fn)(struct dwarf_nebula *, const uint8_t *, ssize_t, ssize_t,
	    void *), void *);

int	dwarf_addr2line(uint64_t, struct dwarf_nebula *,
	    const char **, const char **, int *);
int	dwarf_addr2name(uint64_t,struct dwarf_nebula*,const char**,uint64_t*);

int	dwarf_names_index(struct dwarf_nebula *);

#endif /* _LIBELF_H_ */
