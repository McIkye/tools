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

#include <sys/queue.h>
#include <sys/tree.h>

/* section names not yet in exec_elf.h */
#define	ELF_NOTE	".note.aeriebsd.ident"
#define	ELF_EH_FRAME	".eh_frame"
#define	ELF_EH_FRAME_H	".eh_frame_hdr"
#define	ELF_GCC_EXCEPT	".gcc_except_table"
#define	ELF_GCC_LINK1T	".gnu.linkonce.t"
#define	ELF_GCC_LINK1R	".gnu.linkonce.r"
#define	ELF_GCC_LINK1D	".gnu.linkonce.d"
#define	ELF_GCC_LINK1B	".gnu.linkonce.b"
#define	ELF_STAB	".stab"
#define	ELF_STABSTR	".stabstr"
#define	ELF_STAB_EXCL	".stab.excl"
#define	ELF_STAB_EXCLS	".stab.exclstr"
#define	ELF_STAB_INDEX	".stab.index"
#define	ELF_STAB_IDXSTR	".stab.indexstr"
#define	ELF_STAB_COMM	".comment"

#define	LD_INTERP	"/usr/libexec/ld.so"

#define	ELF_IBUFSZ	0x10000
#define	ELF_OBUFSZ	0x10000
#define	ELF_SBUFSZ	0x200	/* XXX make bigger later */

#define	SHALIGN(a)	(((a) + 15) & ~15)

/* this is used for library path and -L */
struct pathlist {
	TAILQ_ENTRY(pathlist) pl_entry;
	const char *pl_path;
};

struct ldorder;
struct objlist;
struct section;
struct symlist;
struct xreflist;
TAILQ_HEAD(headorder, ldorder);

typedef int (*ordprint_t)(const struct ldorder *, void *);
typedef int (*symprint_t)(const struct ldorder *, const struct section *,
    struct symlist *, void *);

/*
 * this describes one symbol and links it to both
 * global symbol table and per-object list.
 * used for both defined and undefined symbols.
 * once symbol is defined sl_obj is filled;
 * until then sl_obj points to the first referer object;
 * it's important to keep the pointer as it is already
 * used for reference in relocs and possibly other places.
 */
struct symlist {
	TAILQ_HEAD(, xreflist) sl_xref;	/* xref list */
	SPLAY_ENTRY(symlist) sl_node;	/* global symbol table */
	TAILQ_ENTRY(symlist) sl_entry;	/* list per section */
	union {
		Elf32_Sym sym32;
		Elf64_Sym sym64;
	} sl_elfsym;
	struct section *sl_sect;	/* section where defined */
	const char *sl_name;
	long sl_next;			/* uniq local name counter */
};
extern struct symlist *sentry;

struct xreflist {
	TAILQ_ENTRY(xreflist) xl_entry;	/* xref list for each symbol */
	struct objlist *xl_obj;
};

/*
 * relocation description;
 * created for both rel and rela types.
 * links to the symbol once the symbol table has been loaded.
 */
struct relist {
	uint64_t rl_addr;
	int64_t rl_addend;
	struct symlist *rl_sym;		/* referene symbol */
	u_char rl_type;			/* relocation type */
};

/*
 * a section from one object;
 * for progbits sections relocations are loaded if available;
 * array of sections later link-listed into the order for loading.
 */
struct section {
	TAILQ_ENTRY(section) os_entry;	/* list in the order */
	TAILQ_HEAD(, symlist) os_syms;	/* all defined syms in the section */

	off_t os_off;			/* source section offset */
	struct objlist *os_obj;		/* back-ref to the object */
	const char *os_name;		/* section name */
	void *os_sect;			/* elf section descriptor */
	struct relist *os_rels;		/* array of relocations */
	struct relist *os_rp;		/* current rel pointer */
	int os_nrls;			/* number of relocations */
	int os_no;			/* elf section number */
	int os_flags;
#define	SECTION_ORDER	0x10000000	/* pulled into some order */
#define	SECTION_LOADED	0x20000000	/* been loaded */
#define	SECTION_USED	0x40000000	/* syms in this section's been refed */
#define	SECTION_64	0x80000000
};

/*
 * an object either a file or a library member;
 * contains an array of sections and needed elf headers;
 * lists all defined symbols from this object.
 */
struct objlist {
	TAILQ_ENTRY(objlist) ol_entry;	/* global list of objects */
	union {
		struct exec aout;
		Elf32_Ehdr elf32;
		Elf64_Ehdr elf64;
	} ol_hdr;
	off_t ol_off;			/* offset in the library */
	const char *ol_path;		/* path to the file */
	const char *ol_name;		/* path and name of the lib member */
	void *ol_sects;			/* array of section headers */
	char *ol_snames;		/* sections names */
	struct section *ol_bss;		/* a ref to good ol' .bss */
	struct section *ol_sections;	/* array of section descriptors */
	void *ol_aux;			/* aux data (such as phdrs/stab/etc) */
	int ol_naux;			/* items in the aux data */
	int ol_nsect;			/* number of sections */
	int ol_flags;
#define	OBJ_SYSTEM	0x0001
};

/*
 * this describes one arch;
 */
struct ldarch {
	int	la_mach;
	int	la_class;
	const struct ldorder *la_order;
	int	(*la_fix)(off_t, struct section *, char *, int);
};
extern const struct ldarch ldarchs[];
extern const int ldnarch;
extern const struct ldarch *ldarch;

/*
 * loading order descriptor; dual-use;
 * statically defined orders are specifications for loading;
 * according to the specification a dynamis order is created
 * containing lists of sections from all objects to be loaded;
 * special orders allow point and other expression calculation.
 */
struct ldorder {
	enum  {
		ldo_kaput, ldo_option, ldo_expr, ldo_section, ldo_symbol,
		ldo_interp, ldo_ehfrh, ldo_shstr, ldo_symtab, ldo_strtab
	}	ldo_order;
	const char *ldo_name;	/* name of the section or global */
	int ldo_type;		/* type of the section or global */
	int ldo_shflags;
	int ldo_flags;
#define	LD_NOZMAGIC	0x0001	/* ignore for zmagic */
#define	LD_NONMAGIC	0x0002	/* ignore for nmagic */
#define	LD_NOOMAGIC	0x0004	/* ignore for omagic */
#define	LD_WXORX	0x0008	/* only for (normal) W^X executables */
#define	LD_PIE		0x0010	/* for position-independant executables */
#define	LD_DYNAMIC	0x0020	/* for dynamic executables */
#define	LD_EXCTBL	0x0040	/* gcc exception table */
#define	LD_DEBUG	0x0100	/* debugging info (strip w/ -S) */
#define	LD_CONTAINS	0x0200	/* contents is generated in ldo_wurst */
#define	LD_SYMTAB	0x0400	/* this is a symtab or relevant section */
#define	LD_ENTRY	0x1000	/* entry point */
#define	LD_LINK1	0x2000	/* link once bits */
#define	LD_IGNORE	0x4000	/* ignore this entry and whatever matches */
#define	LD_USED		0x8000	/* mark all collected sections as used */
	uint64_t ldo_filler;	/* gap filler */

	TAILQ_HEAD(, section) ldo_seclst;	/* all sections */
	TAILQ_ENTRY(ldorder) ldo_entry;		/* list of the order */
	uint64_t ldo_start;	/* start of this segment */
	uint64_t ldo_addr;	/* current address during section mapping */
	const struct ldarch *ldo_arch;		/* point back to the arch */
	struct section *ldo_sect;		/* output section header */
	void *ldo_wurst;	/* contents to dump thru */
	size_t ldo_wsize;	/* wurst size */
	int ldo_sno;		/* section number for this order */
};

extern struct objlist sysobj;
extern const char *entry_name;
extern char *mapfile;
extern struct ldorder *bsorder;
extern int Xflag, errors, printmap, cref, relocatable, strip, warncomm;
extern int machine, endian, elfclass, magic, pie, Bflag, gc_sections;
extern u_int64_t start_text, start_data, start_bss;
extern const struct ldorder
    alpha_order[], amd64_order[], arm_order[], hppa_order[],
    i386_order[], m68k_order[], mips_order[], ppc_order[],
    sh_order[], sparc_order[], sparc64_order[], vax_order[];
int amd64_fix(off_t, struct section *, char *, int);
int amd64_fixone(char *, uint64_t, int64_t, uint);
int arm_fix(off_t, struct section *, char *, int);
int arm_fixone(char *, uint64_t, int64_t, uint);
int hppa_fix(off_t, struct section *, char *, int);
int hppa_fixone(char *, uint64_t, int64_t, uint);
int i386_fix(off_t, struct section *, char *, int);
int i386_fixone(char *, uint64_t, int64_t, uint);
int sparc64_fix(off_t, struct section *, char *, int);
int sparc64_fixone(char *, uint64_t, int64_t, uint);

const struct ldarch *ldinit(void);
int obj_foreach(int (*)(struct objlist *, void *), void *);
struct headorder *elf_gcs(struct headorder *);

/* ld2.c */
int uLD32(const char *, char *, int *, int);
int uLD64(const char *, char *, int *, int);
struct symlist *elf32_absadd(const char *, int);
struct symlist *elf64_absadd(const char *, int);
int elf32_symadd(struct elf_symtab *, int, void *, void *);
int elf64_symadd(struct elf_symtab *, int, void *, void *);
int elf32_loadrelocs(struct objlist *, struct section *, Elf32_Shdr *,
    FILE *, off_t);
int elf64_loadrelocs(struct objlist *, struct section *, Elf64_Shdr *,
    FILE *, off_t);
int elf32_objadd(struct objlist *, FILE *, off_t);
int elf64_objadd(struct objlist *, FILE *, off_t);
int ld32order_obj(struct objlist *, void *);
int ld64order_obj(struct objlist *, void *);
struct ldorder *ldmap32(struct headorder *);
struct ldorder *ldmap64(struct headorder *);
int ldmap32_obj(struct objlist *, void *);
int ldmap64_obj(struct objlist *, void *);
int ldload32(const char *, struct ldorder *);
int ldload64(const char *, struct ldorder *);
int ldloadasect32(FILE *, FILE *, const char *, const struct ldorder *, struct section *);
int ldloadasect64(FILE *, FILE *, const char *, const struct ldorder *, struct section *);
int elf32_ld_chkhdr(const char *, Elf32_Ehdr *, int, int *, int *, int *);
int elf64_ld_chkhdr(const char *, Elf64_Ehdr *, int, int *, int *, int *);

/* syms.c */
struct symlist *sym_undef(const char *);
struct symlist *sym_isundef(const char *);
struct symlist *sym_define(struct symlist *, struct section *, void *);
struct symlist *sym_redef(struct symlist *, struct section *, void *);
struct symlist *sym_add(const char *, struct section *, void *);
struct symlist *sym_isdefined(const char *, struct section *);
void sym_remove(struct symlist *);
void sym_scan(const struct ldorder *, ordprint_t, symprint_t, void *);
int sym_undcheck(void);
int rel_addrcmp(const void *, const void *);
struct ldorder *order_clone(const struct ldarch *, const struct ldorder *);
void sym_printmap(struct headorder *, ordprint_t, symprint_t);
int order_printmap(const struct ldorder *, void *);
int randombit(void);
char *__cxa_demangle(const char *, char *, size_t *, int *);
