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

#include <sys/types.h>
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <elf_abi.h>
#include <dwarf.h>
#include "elfuncs.h"
#include "elfswap.h"

uint16_t
dwarf_fix16(struct dwarf_nebula *dn, uint16_t a16)
{
	if (dn->elfdata == ELF_TARG_DATA)
		return a16;

	return swap16(a16);
}

uint32_t
dwarf_fix32(struct dwarf_nebula *dn, uint32_t a32)
{
	if (dn->elfdata == ELF_TARG_DATA)
		return a32;

	return swap32(a32);
}

uint64_t
dwarf_fix64(struct dwarf_nebula *dn, uint64_t a64)
{
	if (dn->elfdata == ELF_TARG_DATA)
		return a64;

	return swap64(a64);
}

int
dwarf_ilen(struct dwarf_nebula *dn, const uint8_t **p, ssize_t *len,
    uint64_t *rlen, int *is64)
{
	uint64_t a64;
	uint32_t a32;

	if (*len < 4)
		return -1;
	*is64 = 4;
	memcpy(&a32, *p, sizeof a32);
	a32 = dwarf_fix32(dn, a32);
	*len -= 4;
	*p += 4;
	if (a32 == 0xffffffff) {
		if (*len < 8)
			return -1;
		memcpy(&a64, *p, sizeof a64);
		a64 = dwarf_fix64(dn, a64);
		*rlen = a64;
		*len -= 8;
		*p += 8;
		*is64 = 8;
	} else if (a32 >= 0xffffff00) {
		return -1;
	} else
		*rlen = a32;

	return 0;
}

/* Read a DWARF LEB128 (little-endian base-128) value. */
/* stolen with mods from matthew@dempsky.org */
int
dwarf_leb128(uint64_t *v, const uint8_t **p, ssize_t *len, int sign)
{
	uint64_t rv;
	int shift;
	uint8_t c;

	for (rv = 0, shift = 0; shift < 64 && *len > 0; ) {
		rv |= (uint64_t)((c = *(*p)++) & 0x7f) << shift;
		shift += 7;
		(*len)--;
		if ((c & 0x80) == 0) {
			if (sign && shift < 64 && (c & 0x40) != 0)
				rv |= ~(uint64_t)0 << shift;
			*v = rv;
			return 0;
		}
	}

	return -1;
}

