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
    "$ABSD: dwarf_info.c,v 1.2 2014/08/12 11:39:49 mickey Exp $";
#endif /* not lint */

#include <sys/types.h>
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf_abi.h>
#include <dwarf.h>
#include "elfuncs.h"
#include "elfswap.h"

int
dwarf_attr(struct dwarf_nebula *dn, const uint8_t **cu, ssize_t *len,
    void *v, ssize_t *rlen, int f)
{
	const uint8_t *p, *q;
	uint64_t u64;
	uint32_t u32;
	uint16_t u16;
	ssize_t s;

	switch (f) {
	case DW_FORM_ref_addr:
	case DW_FORM_addr:
		if (*len < dn->a64) {
	trunc:
			warnx("%s: truncated " DWARF_INFO, dn->name);
			return -1;
		}
		s = dn->a64;

		if (v) {
			if (s > *rlen) {
				warnx("s-botch");
				return -1;
			}

			if (dn->a64 == 8) {
				memcpy(&u64, *cu, 8);
				u64 = dwarf_fix64(dn, u64);
			} else {
				memcpy(&u32, *cu, 4);
				u64 = dwarf_fix32(dn, u32);
			}

			/* numbers are always assumed to be u64 */
			*(uint64_t *)v = u64;
		}
		break;

	case DW_FORM_block2:
		if (*len < 2)
			goto trunc;
		goto block;

	case DW_FORM_block4:
		if (*len < 4)
			goto trunc;
		goto block;

	case DW_FORM_data2:
	case DW_FORM_ref2:
		if (*len < 2)
			goto trunc;
		if (v) {
			memcpy(&u16, *cu, 2);
			*(uint64_t *)v = dwarf_fix16(dn, u16);
		}
		s = 2;
		break;

	case DW_FORM_data4:
	case DW_FORM_ref4:
		if (*len < 4)
			goto trunc;
		if (v) {
			memcpy(&u32, *cu, 4);
			*(uint64_t *)v = dwarf_fix32(dn, u32);
		}
		s = 4;
		break;

	case DW_FORM_data8:
	case DW_FORM_ref8:
		if (*len < 8)
			goto trunc;
		if (v) {
			memcpy(&u64, *cu, 8);
			*(uint64_t *)v = dwarf_fix64(dn, u64);
		}
		s = 8;
		break;

	case DW_FORM_string:
		if (!(p = memchr(*cu, '\0', *len)))
			goto trunc;
		s = p - *cu + 1;
		if (rlen && s < *rlen) {
			if (v)
				memcpy(v, *cu, s);
			*rlen = s;
		}
		break;

	case DW_FORM_block:
		if (dwarf_leb128(&u64, cu, len, 0))
			goto trunc;
		goto block;

	case DW_FORM_block1:
		if (*len < 1)
			goto trunc;
		u64 = *(*cu)++;
		(*len)++;

	block:
		if (*len < u64)
			goto trunc;

		if (v)
			memcpy(v, *cu, u64);
		s = (ssize_t)u64;
		break;

	case DW_FORM_data1:
	case DW_FORM_flag:
	case DW_FORM_ref1:
		if (*len < 1)
			goto trunc;
		if (v)
			*(uint64_t *)v = **cu;
		s = 1;
		break;

	case DW_FORM_sdata:
		if (dwarf_leb128(&u64, cu, len, 0))
			goto trunc;
		if (v)
			*(uint64_t *)v = dwarf_fix64(dn, u64);
		s = 0;
		break;

	case DW_FORM_strp:
		if (*len < dn->is64)
			goto trunc;
// memcpy(&u64, *cu, dn->is64);
// u64 = dwarf_fix64(dn, u64);
// fprintf(stderr, "strp 0x%llx/%d\n", u64, dn->is64);
		if (rlen) {
			memcpy(&u64, *cu, dn->is64);
			u64 = dwarf_fix64(dn, u64);
			if (u64 >= dn->nstr) {
				warnx("%s: invalid string index", dn->name);
				return -1;
			}
			p = dn->str + u64;
			s = dn->nstr - u64;
			if (!(q = memchr(p, '\0', s)))
				goto trunc;
			if (v && *rlen < q - p)
				memcpy(v, p, q - p);
			*rlen = q - p + 1;
		}
		s = dn->is64;
		break;

	case DW_FORM_udata:
	case DW_FORM_ref_udata:
		if (dwarf_leb128(&u64, cu, len, 0))
			goto trunc;
		if (v)
			*(uint64_t *)v = dwarf_fix64(dn, u64);
		s = 0;
		break;

	case DW_FORM_indirect:
	default:
		warnx("%s: unknown format %d", dn->name, f);
		return -1;
	}

	if (rlen)
		*rlen = s;
	*cu += s;
	*len -= s;
	return 0;
}

int
dwarf_scan_count(struct dwarf_nebula *dn, const uint8_t *cu, ssize_t len,
    void *v, ssize_t aoff)
{
/* TODO skip NULL units (alignment stubs) */
	(*(ssize_t *)v)++;
	return 1;
}

ssize_t
dwarf_info_count(struct dwarf_nebula *dn)
{
	ssize_t n = 0;

	if (dwarf_info_scan(dn, dwarf_scan_count, &n) < 0)
		return -1;

	return n;
}

int
dwarf_abbrv_find(struct dwarf_nebula *dn, const uint8_t **ab, ssize_t *lab, int idx)
{
	uint64_t at, u64;

	for (;;) {
		if (dwarf_leb128(&at, ab, lab, 0)) {
	trunc:
			warnx("%s: truncated " DWARF_ABBREV, dn->name);
			return -1;
		}

// fprintf(stderr, "idx %lld\n", at);
		if (at == idx)
			return 0;

		/* tag */
		if (dwarf_leb128(&at, ab, lab, 0))
			goto trunc;

		(*ab)++;
		(*lab)--;

		for (;;) {
			if (dwarf_leb128(&at, ab, lab, 0))
				goto trunc;
// fprintf(stderr, "at 0x%llx\n", at);
	
			if (dwarf_leb128(&u64, ab, lab, 0))
				goto trunc;
// fprintf(stderr, "form 0x%llx\n", u64);

			/* end of record */
			if (!at || !u64)
				break;
		}
	}

	return -1;
}

int
dwarf_scan_lines(struct dwarf_nebula *dn, const uint8_t *cu, ssize_t len,
    void *v, ssize_t aoff)
{
	uint64_t at, u64, high, low;
	struct dwarf_line *ln;
	const uint8_t *ab = (const uint8_t *)dn->abbrv + aoff;
	ssize_t rlen, soff, lab = dn->nabbrv - aoff;

	if (dwarf_leb128(&u64, &cu, &len, 0)) {
		warnx("%s: truncated " DWARF_INFO, dn->name);
		return 0;
	}
// fprintf(stderr, "u64 0x%llx\n", u64);

	/* skip NULL units (alignment stubs) */
	if (u64 == 0)
		return 1;

	if (dwarf_abbrv_find(dn, &ab, &lab, u64)) {
		warnx("%s: corrupt " DWARF_INFO, dn->name);
		return 0;
	}

	if (dwarf_leb128(&u64, &ab, &lab, 0)) {
		warnx("%s: truncated " DWARF_ABBREV, dn->name);
		return 0;
	}
// fprintf(stderr, "tag 0x%llx\n", u64);

/* TODO also parse DW_TAG_partial_unit */
	if (u64 != DW_TAG_compile_unit)
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

	for (high = 0, low = ~0, soff = -1;;) {
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
			if (dwarf_attr(dn, &cu, &len, &low, &rlen, u64))
				return 0;
// fprintf(stderr, "low 0x%llx\n", low);
			break;

		case DW_AT_high_pc:
			rlen = 8;
			if (dwarf_attr(dn, &cu, &len, &high, &rlen, u64))
				return 0;
// fprintf(stderr, "high 0x%llx\n", high);
			break;

		case DW_AT_stmt_list:
			rlen = 8;
			if (dwarf_attr(dn, &cu, &len, &u64, &rlen, u64))
				return 0;
			/* it was probably data4 (: */
			soff = (ssize_t)u64;
// fprintf(stderr, "soff 0x%zd\n", soff);
			break;

		default:
			/* skip otherwise */
			if (dwarf_attr(dn, &cu, &len, NULL, NULL, u64))
				return 0;
		}
	}

	if (low == ~0 || high == 0 || soff < 0) {
		warnx("%s: incomplete " DWARF_ABBREV " record", dn->name);
		return 0;
	}

	if (high < low || soff > dn->nlines) {
		warnx("%s: corrupt " DWARF_INFO, dn->name);
		return 0;
	}

	ln = &dn->a2l[*(ssize_t *)v];
	if (*(ssize_t *)v >= dn->nunits) {
		warnx("%s: inconsistant unit number", dn->name);
		return 0;
	}
	*(ssize_t *)v += 1;	/* this seems to be a ++/cast bug in gcc */

	ln->addr = low;
	ln->len = high - low + 1;
	ln->lnp = (const char *)dn->lines + soff;
// fprintf(stderr, "0x%llx %lld, %p\n", ln->addr, ln->len, ln->lnp);
	return 1;
}

int
dwarf_line_cmp(const void *v1, const void *v2)
{
	const struct dwarf_line *a = v1, *b = v2;

	if (a->addr < b->addr)
		return -1;
	else if (a->addr > b->addr)
		return 1;
	else
		return 0;
}

int
dwarf_info_lines(struct dwarf_nebula *dn)
{
	ssize_t i;

	if (!dn->info || !dn->nunits) {
		warnx("%s: " DWARF_INFO " not loaded", dn->name);
		return -1;
	}

	if (!dn->lines) {
		warnx("%s: " DWARF_LINE " not loaded", dn->name);
		return -1;
	}

	if (!(dn->a2l  = calloc(dn->nunits, sizeof *dn->a2l))) {
		warn("calloc");
		return -1;
	}

	i = 0;
	if (dwarf_info_scan(dn, dwarf_scan_lines, &i) < 0) {
		free(dn->a2l);
		dn->a2l = NULL;
		return -1;
	}

	qsort(dn->a2l, dn->nunits, sizeof *dn->a2l, dwarf_line_cmp);
/* TODO check for overlapping ranges? */
// fprintf(stderr, "0 %llx %lld\n", dn->a2l[0].addr, dn->a2l[0].len);
	return 0;
}

int
dwarf_info_abbrv(struct dwarf_nebula *dn, const uint8_t **cu, ssize_t *len,
    ssize_t *rlen, ssize_t *aoff)
{
	uint64_t a64;
	uint16_t a16;
	int ver;

	if (dwarf_ilen(dn, cu, len, &a64, &dn->is64) || a64 > *len) {
		warnx("%s: corrupt " DWARF_INFO, dn->name);
		return -1;
	}
// fprintf(stderr, "il %lld/%zd\n", a64, *len);
	*rlen = a64;
	*len -= *rlen;

	if (*rlen < 3 + dn->is64) {
		warnx("%s: truncated " DWARF_INFO, dn->name);
		return -1;
	}

	memcpy(&a16, *cu, sizeof a16);
	if ((ver = dwarf_fix16(dn, a16)) != 2) {
		warnx("%s: unsupported DWARF version %d", dn->name, ver);
		return -1;
	}
	*cu += 2;
	*rlen -= 2;

	*aoff = dwarf_off48(dn, cu);
	*rlen -= dn->is64;

// fprintf(stderr, "aoff %zu\n", *aoff);
	if (*aoff >= dn->nabbrv) {
		warnx("%s: corrupt " DWARF_INFO, dn->name);
		return -1;
	}

	dn->a64 = *(*cu)++;
	if (dn->a64 != 4 && dn->a64 != 8) {
		warnx("%s: invalid address len %d", dn->name, dn->a64);
		return -1;
	}
	(*rlen)--;

	return 0;
}

int
dwarf_info_scan(struct dwarf_nebula *dn,
    int (*fn)(struct dwarf_nebula *, const uint8_t *, ssize_t, void *,
    ssize_t), void *v)
{
	const uint8_t *p;
	ssize_t rlen, aoff, len;
	int rv;

	if (!dn->info) {
		warnx("%s: " DWARF_INFO " not loaded", dn->name);
		return -1;
	}

	for (p = dn->info, len = dn->ninfo; len > 0; p += rlen) {
		if (dwarf_info_abbrv(dn, &p, &len, &rlen, &aoff))
			return -1;

		dn->unit = p;
		if ((rv = (*fn)(dn, p, rlen, v, aoff)) <= 0)
			return rv;
	}

	return 1;
}
