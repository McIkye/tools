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
    "$ABSD: dwarf_names.c,v 1.1 2014/08/12 11:39:49 mickey Exp $";
#endif /* not lint */

#include <sys/types.h>
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <elf_abi.h>
#include <dwarf.h>
#include "elfuncs.h"
#include "elfswap.h"

int
dwarf_names_count(struct dwarf_nebula *dn, const uint8_t *fn, ssize_t ioff, ssize_t aoff, void *v)
{
	(*(ssize_t *)v)++;
	return 1;
}

int
dwarf_names_entry(struct dwarf_nebula *dn, const uint8_t *fn, ssize_t ioff, ssize_t aoff, void *v)
{
	uint64_t u64, idx, at, high, low;
	struct dwarf_name *nm;
	const uint8_t *ab = (const uint8_t *)dn->abbrv + aoff;
	ssize_t lab = dn->nabbrv - aoff;
	const uint8_t *cu = dn->unit + ioff;
	ssize_t lu = dn->ninfo - (cu - dn->info);
	ssize_t rlen;

// fprintf(stderr, "n %zd N %zd\n", *(ssize_t *)v, dn->ncount);
	if (*(ssize_t *)v >= dn->ncount) {
		warnx("%s: names botch", dn->name);
		return -1;
	}

	do {
		if (dwarf_leb128(&idx, &cu, &lu, 0)) {
			warnx("%s: truncated " DWARF_INFO, dn->name);
			return 0;
		}

// fprintf(stderr, "idx %lld\n", idx);
		if (idx >= INT_MAX) {
			warnx("%s: corrupt " DWARF_INFO, dn->name);
			return 0;
		}
	} while (!idx);

// fprintf(stderr, "ab %p/%p lab %zd/%zd\n", dn->abbrv, ab, dn->nabbrv, lab);
	if (dwarf_abbrv_find(dn, &ab, &lab, idx))
		return 0;

// fprintf(stderr, "ab %p lab %zd\n", ab, lab);
	if (dwarf_leb128(&u64, &ab, &lab, 0)) {
		warnx("%s: truncated " DWARF_ABBREV, dn->name);
		return 0;
	}
// fprintf(stderr, "tag %lld\n", u64);

	if (u64 != DW_TAG_subprogram && u64 != DW_TAG_variable) {
		warnx("%s: invalid " DWARF_ABBREV " tag %d for \'%s\'",
		    dn->name, (int)u64, fn);
		return 0;
	}

	/* TODO */
	if (u64 == DW_TAG_variable)
		return 1;

	if (lab < 1) {
		warnx("%s: truncated " DWARF_ABBREV, dn->name);
		return 0;
	}

	lab--;
	if (*ab++ != DW_CHILDREN_yes) {
//		warnx("%s: bare compile unit abbreviation", dn->name);
//		return 0;
	}
// fprintf(stderr, "child %d\n", ab[-1]);

	for (high = 0, low = ~0;;) {
		if (dwarf_leb128(&at, &ab, &lab, 0)) {
			warnx("%s: truncated " DWARF_ABBREV, dn->name);
			return 0;
		}
// fprintf(stderr, "at 0x%llx\n", at);

		if (dwarf_leb128(&u64, &ab, &lab, 0)) {
			warnx("%s: truncated " DWARF_ABBREV, dn->name);
			return 0;
		}
// fprintf(stderr, "form 0x%llx\n", u64);

		/* end of record */
		if (!at || !u64)
			break;

		switch (at) {
		case DW_AT_low_pc:
			rlen = 8;
			if (dwarf_attr(dn, &cu, &lu, &low, &rlen, u64))
				return 0;
// fprintf(stderr, "low 0x%llx\n", low);
			break;
		case DW_AT_high_pc:
			rlen = 8;
			if (dwarf_attr(dn, &cu, &lu, &high, &rlen, u64))
			return 0;
// fprintf(stderr, "high 0x%llx\n", high);
			break;

		default:
			/* skip otherwise */
			if (dwarf_attr(dn, &cu, &lu, NULL, NULL, u64))
				return 0;
		}

	}

	if (low == ~0 || high == 0) {
		warnx("%s: incomplete " DWARF_ABBREV " record", dn->name);
		return 0;
	}

	if (high < low) {
		warnx("%s: corrupt " DWARF_INFO, dn->name);
		return 0;
	}

	nm = &dn->a2n[*(ssize_t *)v];
	nm->addr = low;
	nm->len  = high - low;
	nm->name = fn;
	nm->unit = dn->unit;

	nm = &dn->n2a[*(ssize_t *)v];
	nm->addr = low;
	nm->len  = high - low;
	nm->name = fn;
	nm->unit = dn->unit;

	*(ssize_t *)v += 1; /* this seems to be a ++/cast bug in gcc */
	return 1;
}

int
dwarf_addr_cmp(const void *v1, const void *v2)
{
	const struct dwarf_name *a = v1, *b = v2;

	if (a->addr < b->addr)
		return -1;
	else if (a->addr > b->addr)
		return 1;
	else
		return 0;
}

int
dwarf_name_cmp(const void *v1, const void *v2)
{
	const struct dwarf_name *a = v1, *b = v2;

	return strcmp(a->name, b->name);
}

int
dwarf_names_index(struct dwarf_nebula *dn)
{
	struct dwarf_name *a2n, *n2a;
	ssize_t n = 0;

	if (dwarf_names_scan(dn, dwarf_names_count, &n) <= 0)
		return -1;

	if (!(a2n = calloc(n, sizeof *a2n))) {
		warn("%s: calloc a2n", dn->name);
		return -1;
	}

	if (!(n2a = calloc(n, sizeof *n2a))) {
		warn("%s: calloc n2a", dn->name);
		free(a2n);
		return -1;
	}

	dn->ncount = n;
	dn->a2n = a2n;
	dn->n2a = n2a;
	n = 0;
	if (dwarf_names_scan(dn, dwarf_names_entry, &n) <= 0) {
		free(a2n);
		free(n2a);
		dn->a2n = dn->n2a = NULL;
		return -1;
	}
	/* n may change due to names skept */
	dn->ncount = n;

	qsort(a2n, n, sizeof *a2n, dwarf_addr_cmp);
	qsort(n2a, n, sizeof *n2a, dwarf_name_cmp);

	return 0;
}

int
dwarf_addr_canhas(const void *v1, const void *v2)
{
	const struct dwarf_name *k = v1, *a = v2;

	if (k->addr >= a->addr && k->addr < a->addr + a->len)
		return 0;
	else if (k->addr < a->addr)
		return -1;
	else
		return 0;
}

int
dwarf_addr2name(uint64_t addr, struct dwarf_nebula *dn, const char **fn,
    uint64_t *aoff)
{
	struct dwarf_name k, *nm;

	nm = dn->a2n;
	k.addr = addr;
	if (!(nm = bsearch(&k, nm, dn->ncount, sizeof *nm, dwarf_addr_canhas))){
		warnx("%s: 0x%llx has no matching name", dn->name, addr);
		return -1;
	}

	*fn = nm->name;
	*aoff = addr - nm->addr;
// fprintf(stderr, "a 0x%llx 0x%llx\n", addr, nm->addr);

	return 0;
}

int
dwarf_names_scan(struct dwarf_nebula *dn,
    int (*fn)(struct dwarf_nebula *, const uint8_t *, ssize_t, ssize_t, void *),
    void *v)

{
	uint64_t a64;
	const uint8_t *p, *er, *q;
	ssize_t ilen, len, rlen, ioff, aoff;
	int is64, ver, rv;
	uint16_t a16;

	if (!dn->info) {
		warnx("%s: " DWARF_INFO " not loaded", dn->name);
		return -1;
	}

	if (!dn->names) {
		warnx("%s: " DWARF_PUBNAMES " not loaded", dn->name);
		return -1;
	}

	for (p = dn->names, len = dn->nnames; len > 0; p = er) {
		if (dwarf_ilen(dn, &p, &len, &a64, &is64) || a64 > len) {
		       warnx("%s: corrupt " DWARF_PUBNAMES, dn->name);
		       return 1;
		}
		er = p + a64;
		len -= a64;

// fprintf(stderr, "il %lld\n", a64);
		if (er - p < 2 + 2 * is64) {
	trunc:
			warnx("%s: truncated " DWARF_PUBNAMES, dn->name);
			return -1;
		}

		memcpy(&a16, p, sizeof a16);
		if ((ver = dwarf_fix16(dn, a16)) != 2) {
			warnx("%s: unsupported DWARF version %d", dn->name, ver);
			return -1;
		}
		p += 2;

		dn->is64 = is64;
		ioff = dwarf_off48(dn, &p);
// fprintf(stderr, "ioff %zd\n", ioff);
		if (ioff >= dn->ninfo) {
			warnx("%s: corrupt " DWARF_PUBNAMES, dn->name);
			return -1;
		}

		ilen = dwarf_off48(dn, &p);
// fprintf(stderr, "ilen %zd\n", ilen);
		if (ilen > dn->ninfo - ioff) {
			warnx("%s: corrupt " DWARF_PUBNAMES, dn->name);
			return -1;
		}

		q = dn->unit = dn->info + ioff;
		if (dwarf_info_abbrv(dn, &q, &ilen, &rlen, &aoff) < 0)
			return -1;
// fprintf(stderr, "aoff %zd rlen %zd\n", ioff, rlen);

		dn->unit = dn->info + ioff;
		do {
			if (er - p < is64)
				goto trunc;

			/* off into a comp-unit inside .debug_info */
			ioff = dwarf_off48(dn, &p);
// fprintf(stderr, "ioff %zd rlen %zd\n", ioff, rlen);
			if (ioff >= rlen)
				break;

			if (!ioff)
				break;

// fprintf(stderr, "%p %p %s\n", p, er, p);
			if (!(q = memchr(p, '\0', er - p)))
				goto trunc;

			if ((rv = (*fn)(dn, p, ioff, aoff, v)) <= 0)
				return rv;
			p = q + 1;
		} while (p < er);
// fprintf(stderr, "len %zd\n", len);
	}

	return 1;
}
