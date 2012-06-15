/*
 * Copyright (c) 2012 Michael Shalayeff
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

extern
struct sym {
        union {
		Elf32_Sym sym32;
		Elf64_Sym sym64;
	} sl_elfsym;
	const char *sl_name;
} *symidx;
extern int nsyms;

const char *elf_type(int);
const char *elf_machine(int);
const char *elf_osabi(int);
const char *elf_pflags(int);
const char *elf_sflags(int);
const char *elf_phtype(int);
const char *elf_shtype(int);
const char *elf_shndx(int);
const char *elf_symbind(int);
const char *elf_symtype(int);
const char *elf_reltype(int, int);

int elf32_gethdrs(FILE *, const char *, off_t, Elf32_Ehdr *, void **, void **, void **);
int elf64_gethdrs(FILE *, const char *, off_t, Elf64_Ehdr *, void **, void **, void **);
int elf32_verify(Elf32_Ehdr *, void *, void *, const char *);
int elf64_verify(Elf64_Ehdr *, void *, void *, const char *);
void elf32_prhdr(Elf32_Ehdr *);
void elf64_prhdr(Elf64_Ehdr *);
void elf32_prsegs(Elf32_Ehdr *, void *);
void elf64_prsegs(Elf64_Ehdr *, void *);
void elf32_prsechs(Elf32_Ehdr *, void *, const char *);
void elf64_prsechs(Elf64_Ehdr *, void *, const char *);
void elf32_prsyms(Elf32_Ehdr *, void *, const char *);
void elf64_prsyms(Elf64_Ehdr *, void *, const char *);
void elf32_prels(FILE*, const char*, off_t, Elf32_Ehdr *, void *, const char*);
void elf64_prels(FILE*, const char*, off_t, Elf64_Ehdr *, void *, const char*);

extern int opts;
#define	ALL	0x00000ff
#define	FILEH	0x0000001
#define	PROGH	0x0000002
#define	SECH	0x0000004
#define	HDRS	0x0000007
#define	SYMS	0x0000010
#define	NOTES	0x0000020
#define	RELOCS	0x0000040
#define	VERS	0x0000080
#define	UNWIND	0x0000100
#define	DYNS	0x0000200
#define	ARCHS	0x0000400
#define	USEDYN	0x0000800
#define	HEX	0x0001000
#define	DEBUG	0x0ffc000
#define	LINE	0x0010000
#define	INFO	0x0020000
#define	LNAME	0x0040000
#define	GNAMES	0x0080000
#define	RANGES	0x0100000
#define	MACRO	0x0200000
#define	FRAMES	0x0400000
#define	FRMINT	0x0800000
#define	STRINGS	0x0008000
#define	LOCS	0x0004000
#define	VERIFY	0x2000000
#define	HIST	0x4000000
#define	WIDE	0x8000000

