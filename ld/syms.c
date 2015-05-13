/*
 * Copyright (c) 2009-2012 Michael Shalayeff
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
    "$ABSD: syms.c,v 1.13 2011/07/23 17:09:41 mickey Exp $";
#endif

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf_abi.h>
#include <elfuncs.h>
#include <a.out.h>
#include <errno.h>
#include <err.h>

#include "ld.h"

static inline int
symcmp(struct symlist *a, struct symlist *b)
{
	return strcmp(a->sl_name, b->sl_name);
}

RB_HEAD(symtree, symlist) undsyms = RB_INITIALIZER(undsyms),
    defsyms = RB_INITIALIZER(defsyms);

RB_PROTOTYPE(symtree, symlist, sl_node, symcmp);

RB_GENERATE(symtree, symlist, sl_node, symcmp);

/*
 * add and return a symbol as undefined
 */
struct symlist *
sym_undef(const char *name)
{
	struct symlist *sym;

	if (!(sym = calloc(1, sizeof *sym)))
		err(1, "calloc");
	TAILQ_INIT(&sym->sl_xref);

	if (!(sym->sl_name = strdup(name)))
		err(1, "strdup");

	RB_INSERT(symtree, &undsyms, sym);
	return sym;
}

/*
 * check and return for symbol a known undefined
 */
struct symlist *
sym_isundef(const char *name)
{
	struct symlist key;

	bzero(&key, sizeof key);
	key.sl_name = name;
	return RB_FIND(symtree, &undsyms, &key);
}

/*
 * define and return a previously undefined symbol
 */
struct symlist *
sym_define(struct symlist *sym, struct section *os, void *esym)
{
	RB_REMOVE(symtree, &undsyms, sym);
	sym->sl_sect = os;
	memcpy(&sym->sl_elfsym, esym, sizeof sym->sl_elfsym);
	RB_INSERT(symtree, &defsyms, sym);
	/* ABS symbols have no section */
	if (os)
		TAILQ_INSERT_TAIL(&os->os_syms, sym, sl_entry);
	return sym;
}

/*
 * redefine a symbol (used for commons)
 */
struct symlist *
sym_redef(struct symlist *sym, struct section *os, void *esym)
{
	if (sym->sl_sect)
		TAILQ_REMOVE(&sym->sl_sect->os_syms, sym, sl_entry);
	sym->sl_sect = os;
	if (esym)
		memcpy(&sym->sl_elfsym, esym, sizeof sym->sl_elfsym);
	TAILQ_INSERT_TAIL(&os->os_syms, sym, sl_entry);
	return sym;
}

/*
 * add a new symbol as defined
 */
struct symlist *
sym_add(const char *name, struct section *os, void *esym)
{
	struct symlist *sym;

	if (!(sym = calloc(1, sizeof *sym)))
		err(1, "calloc");
	TAILQ_INIT(&sym->sl_xref);

	if (!(sym->sl_name = strdup(name)))
		err(1, "strdup");

	sym->sl_sect = os;
	memcpy(&sym->sl_elfsym, esym, sizeof sym->sl_elfsym);
	RB_INSERT(symtree, &defsyms, sym);
	/* ABS symbols have no section */
	if (os)
		TAILQ_INSERT_TAIL(&os->os_syms, sym, sl_entry);
	return sym;
}

/*
 * check and return for symbol being defined
 */
struct symlist *
sym_isdefined(const char *name, struct section *os)
{
	struct symlist key, *sym;

	bzero(&key, sizeof key);
	key.sl_name = name;
	if (!(sym = RB_FIND(symtree, &defsyms, &key)) && os) {
		key.sl_sect = os;
		sym = RB_FIND(symtree, &defsyms, &key);
	}

	return sym;
}

/*
 * remove the symbol and all its gedoens from
 * the global symbol table of defined symbols
 * (used by the relocs optimisations)
 */
void
sym_remove(struct symlist *sym)
{
	struct xreflist *xl;

	if (sym->sl_sect)
		TAILQ_REMOVE(&sym->sl_sect->os_syms, sym, sl_entry);
	RB_REMOVE(symtree, &defsyms, sym);
	free((char *)sym->sl_name);	/* it was const */
	/* only when cref is set */
	while (!TAILQ_EMPTY(&sym->sl_xref)) {
		xl = TAILQ_FIRST(&sym->sl_xref);
		TAILQ_REMOVE(&sym->sl_xref, xl, xl_entry);
		free(xl);
	}
}

/*
 * if there are any undefined symbols left
 * report 'em now
 */
int
sym_undcheck(void)
{
	struct symlist *sym;
	int err = 0;

	RB_FOREACH(sym, symtree, &undsyms) {
		if (!sym->sl_name)
			continue;
		if (sym->sl_sect)
			warnx("%s: undefined, first used in %s",
			    sym->sl_name,
			    sym->sl_sect->os_obj->ol_name);
		else
			warnx("%s: undefined", sym->sl_name);
		err = -1;
	}

	return err;
}

/*
 * print out the map of the final executable
 * if requested also dump out the cross-reference table
 */
void
sym_printmap(struct headorder *headorder, ordprint_t of, symprint_t sf)
{
	FILE *mfp;

	if (!mapfile)
		mfp = stdout;
	else {
		if (!(mfp = fopen(mapfile, "w")))
			err(1, "fopen: %s", mapfile);
	}

	sym_scan(TAILQ_FIRST(headorder), of, sf, mfp);

	if (cref) {
		struct symlist *sym;

		fputs("\nCross reference table:\n\n", mfp);
		RB_FOREACH(sym, symtree, &defsyms) {
			struct xreflist *xl;
			int first = 1;

			TAILQ_FOREACH(xl, &sym->sl_xref, xl_entry) {
				if (first) {
					fprintf(mfp, "%-16s", sym->sl_name);
					first = 0;
				} else
					fprintf(mfp, "\t\t");

				fprintf(mfp, "\t%s\n", xl->xl_obj->ol_name);
			}
		}
	}

	if (mapfile)
		fclose(mfp);
	else
		fflush(mfp);
}

/*
 * call a given function over all symbols defined to date
 * used by the strtab generation and map printing
 */
void
sym_scan(const struct ldorder *order, ordprint_t of, symprint_t sf, void *v)
{
	for(; order != TAILQ_END(NULL); order = TAILQ_NEXT(order, ldo_entry)) {
		struct section *os;

		if (of && (*of)(order, v))
			return;

		TAILQ_FOREACH(os, &order->ldo_seclst, os_entry) {
			struct symlist *sym;

			TAILQ_FOREACH(sym, &os->os_syms, sl_entry)
				if (sf && (*sf)(order, os, sym, v))
					return;
		}
	}
}

/*
 * these are comparison functions used to sort relocation
 * array in elf_loadrelocs()
 */
int
rel_addrcmp(const void *a0, const void *b0)
{
	const struct relist *a = a0, *b = b0;

	if (a->rl_addr < b->rl_addr)
		return -1;
	else if (a->rl_addr > b->rl_addr)
		return 1;
	else
		return 0;
}

/*
 * allocate new order piece cloning from the templar
 */
struct ldorder *
order_clone(const struct ldarch *lda, const struct ldorder *order)
{
	struct ldorder *neworder;

	if ((neworder = calloc(1, sizeof *neworder)) == NULL)
		err(1, "calloc");

	TAILQ_INIT(&neworder->ldo_seclst);
	neworder->ldo_order = order->ldo_order;
	neworder->ldo_name = order->ldo_name;
	neworder->ldo_type = order->ldo_type;
	neworder->ldo_flags = order->ldo_flags;
	neworder->ldo_shflags = order->ldo_shflags;
	neworder->ldo_arch = lda;

	return neworder;
}

/*
 * map printing function called for each order member
 * that is only sections matter for now
 */
int
order_printmap(const struct ldorder *order, void *v)
{
	switch (order->ldo_order) {
	case ldo_expr:
		break;

	case ldo_symbol:
		fprintf(v, "%-16s 0x%08llx\n",
		    order->ldo_name, order->ldo_start);
		break;

	default:
		fprintf(v, "%-16s 0x%08llx\t0x%llx\n", order->ldo_name,
		    order->ldo_start, order->ldo_addr - order->ldo_start);
	}

	return 0;
}

/*
 * return a random bit 0/1
 * used for object order randomisation
 */
int
randombit(void)
{
	static uint32_t b;
	static int i;

	if (i == 0) {
		b = arc4random();
		i = 31;
	}

	return b & (1 << i--);
}
