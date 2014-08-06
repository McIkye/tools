/*
 * Copyright (c) 2014 Matthew Dempsky <matthew@dempsky.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

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

#define DWARN warnx

struct dwbuf {
	const uint8_t *buf;
	ssize_t len;
};

int
dwarf_a2l_unit(uint64_t, struct dwarf_nebula *, struct dwarf_line *,
    const char **, const char **, int *);

int
read_bytes(struct dwbuf *d, void *v, size_t n)
{
	if (d->len < n)
		return (0);
	memcpy(v, d->buf, n);
	d->buf += n;
	d->len -= n;
	return (1);
}

int
read_s8(struct dwbuf *d, int8_t *v)
{
	return (read_bytes(d, v, sizeof(*v)));
}

int
read_u8(struct dwbuf *d, uint8_t *v)
{
	return (read_bytes(d, v, sizeof(*v)));
}

int
read_u16(struct dwbuf *d, uint16_t *v)
{
	return (read_bytes(d, v, sizeof(*v)));
}

int
read_u32(struct dwbuf *d, uint32_t *v)
{
	return (read_bytes(d, v, sizeof(*v)));
}

int
read_u64(struct dwbuf *d, uint64_t *v)
{
	return (read_bytes(d, v, sizeof(*v)));
}

/* Read a NUL terminated string. */
int
read_string(struct dwbuf *d, const char **s)
{
	const uint8_t *end = memchr(d->buf, '\0', d->len);
	size_t n;
	if (end == NULL)
		return (0);
	n = end - d->buf + 1;
	*s = (const char *)d->buf;
	d->buf += n;
	d->len -= n;
	return (1);
}

static inline int
read_sleb128(struct dwbuf *d, int64_t *v)
{
	return !dwarf_leb128(v, &d->buf, &d->len, 1);
}

static inline int
read_uleb128(struct dwbuf *d, uint64_t *v)
{
	return !dwarf_leb128(v, &d->buf, &d->len, 0);
}

int
read_buf(struct dwbuf *d, struct dwbuf *v, size_t n)
{
	if (d->len < n)
		return (0);
	v->buf = d->buf;
	v->len = n;
	d->buf += n;
	d->len -= n;
	return (1);
}

int
skip_bytes(struct dwbuf *d, size_t n)
{
	if (d->len < n)
		return (0);
	d->buf += n;
	d->len -= n;
	return (1);
}

int
read_filename(struct dwbuf *names, const char **outdirname,
    const char **outbasename, uint8_t opcode_base, uint64_t file)
{
	struct dwbuf dirnames = *names;
	const char *basename = NULL;
	const char *dirname = NULL;
	uint64_t dir = 0;
	size_t i;

	if (file == 0)
		return -1;

	/* Skip over opcode table. */
	for (i = 1; i < opcode_base; i++) {
		uint64_t dummy;
		if (!read_uleb128(names, &dummy))
			return -1;
	}

	/* Skip over directory name table for now. */
	for (;;) {
		const char *name;
		if (!read_string(names, &name))
			return -1;
		if (*name == '\0')
			break;
	}

	/* Locate file entry. */
	basename = NULL;
	for (i = 0; i < file; i++) {
		uint64_t mtime, size;
		if (!read_string(names, &basename) || *basename == '\0' ||
		    !read_uleb128(names, &dir) ||
		    !read_uleb128(names, &mtime) ||
		    !read_uleb128(names, &size))
			return -1;
	}

	for (i = 0; i < dir; i++) {
		if (!read_string(&dirnames, &dirname) || *dirname == '\0')
			return -1;
	}

	*outdirname = dirname;
	*outbasename = basename;
	return (0);
}

int
dwarf_line_canhas(const void *v1, const void *v2)
{
	const struct dwarf_line *k = v1, *a = v2;

	if (k->addr >= a->addr && k->addr < a->addr + a->len)
		return 0;
	else if (k->addr < a->addr)
		return -1;
	else
		return 1;
}

int
dwarf_addr2line(uint64_t pc, struct dwarf_nebula *dn,
    const char **pdir, const char **pfname, int *pln)
{
	struct dwarf_line k, *ln;

	if (!dn->info) {
		warnx("%s: " DWARF_INFO " not loaded", dn->name);
		return 1;
	}

	if (!dn->lines) {
		warnx("%s: " DWARF_LINE " not loaded", dn->name);
		return 1;
	}

	ln = dn->a2l;
	k.addr = pc;
	if (!(ln = bsearch(&k, ln, dn->nunits, sizeof *ln, dwarf_line_canhas)))
		return -1;

// fprintf(stderr, "pc %llx addr %llx %llx\n", pc, ln->addr, ln->len);
	return dwarf_a2l_unit(pc, dn, ln, pdir, pfname, pln);
}

int
dwarf_a2l_unit(uint64_t pc, struct dwarf_nebula *dn, struct dwarf_line *ln,
    const char **pdir, const char **pfname, int *pln)
{
	struct dwbuf headerstart;
	struct dwbuf unit;
	struct dwbuf names;
	uint64_t u64, unitsize;
	uint64_t header_size;
	uint32_t u32;
	uint16_t version;
	uint8_t min_insn_length, default_is_stmt, line_range, opcode_base;
	int8_t line_base;
	const uint8_t *cu = ln->lnp;
	ssize_t len;
	int s;
	/* VM registers. */
	uint64_t address = 0, file = 1, line = 1, column = 0;
	uint8_t is_stmt;
	/* Last line table entry emitted, if any. */
	int have_last = 0;
	uint64_t last_line = 0;
	/* Time to run the line program. */
	uint8_t opcode;

// fprintf(stderr, "ln %p\n", cu);
	s = sizeof unitsize;
	len = 12;	/* XXX as we do not know how much real space... */
	if (dwarf_ilen(dn, &cu, &len, &unitsize, &s))
		return -1;

// fprintf(stderr, "u %lld\n", unitsize);
	unit.buf = cu;
	unit.len = unitsize;

// fprintf(stderr, "ut %p %zd\n", unit.buf, unit.len);
	u64 = u32 = 0;
	if (!read_u16(&unit, &version) || version > 2 ||
	    !((dn->is64 && read_u64(&unit, &u64)) ||
	      (!dn->is64 && read_u32(&unit, &u32))))
		return -1;
	header_size = u32 + u64 - 12;	// XXX XXX XXX

// fprintf(stderr, "v %d %lld\n", version, header_size);
	headerstart = unit;
	if (!read_u8(&unit, &min_insn_length) ||
	    !read_u8(&unit, &default_is_stmt) ||
	    !read_s8(&unit, &line_base) ||
	    !read_u8(&unit, &line_range) ||
	    !read_u8(&unit, &opcode_base))
		return -1;

// fprintf(stderr, "p %d %d %d %d %d\n", min_insn_length, default_is_stmt, line_base, line_range, opcode_base);

	/* skip for now std opcode lengths */
	for (s = opcode_base; s--; )
		read_uleb128(&unit, &u64);

	/*
	 * Directory and file names are next in the header, but for now we
	 * skip directly to the line number program.
	 */
	names = unit;
	unit = headerstart;
	is_stmt = default_is_stmt;
	if (!skip_bytes(&unit, header_size))
		return -1;

// fprintf(stderr, "ut %p %zd\n", unit.buf, unit.len);
	while (read_u8(&unit, &opcode)) {
		int emit = 0;

// fprintf(stderr, "op %d\n", opcode);
		if (opcode >= opcode_base) {
			uint8_t diff = opcode - opcode_base;
// fprintf(stderr, "spop %d\n", opcode);
			/* "Special" opcodes. */
			address += diff / line_range;
			line += line_base + diff % line_range;
			emit = 1;
		} else if (opcode == 0) {
			/* "Extended" opcodes. */
			uint64_t extsize;
			struct dwbuf extra;
// fprintf(stderr, "exlen %d %zd\n", unit.buf[0], unit.len);
			if (!read_uleb128(&unit, &extsize) ||
			    !read_buf(&unit, &extra, extsize) ||
			    !read_u8(&extra, &opcode))
				return -1;
// fprintf(stderr, "exop %d %lld\n", opcode, extsize);
			switch (opcode) {
			case DW_LNE_end_sequence:
				emit = 1;
				break;
			case DW_LNE_set_address:
				switch (extra.len) {
				case 0:
					/* XXX is this the idea? */
					address = ln->addr;
					break;
				case 4: {
					uint32_t address32;
					if (!read_u32(&extra, &address32))
						return -1;
					address = address32;
					break;
				}
				case 8:
					if (!read_u64(&extra, &address))
						return -1;
					break;
				default:
					DWARN("unexpected address length: %zu",
					    extra.len);
					return -1;
				}
				break;
			case DW_LNE_define_file:
				/* XXX: hope this isn't needed */
			default:
				DWARN("unknown extended opcode: %d", opcode);
				/* just continue as chances are we recover */
				break;
			}
		} else {
// fprintf(stderr, "stop %d\n", opcode);
			/* "Standard" opcodes. */
			switch (opcode) {
			case DW_LNS_copy:
				emit = 1;
				break;
			case DW_LNS_advance_pc: {
				uint64_t delta;
				if (!read_uleb128(&unit, &delta))
					return -1;
				address += delta * min_insn_length;
				break;
			}
			case DW_LNS_advance_line: {
				int64_t delta;
				if (!read_sleb128(&unit, &delta))
					return -1;
				line += delta;
				break;
			}
			case DW_LNS_set_file:
				if (!read_uleb128(&unit, &file))
					return -1;
				break;
			case DW_LNS_set_column:
				if (!read_uleb128(&unit, &column))
					return -1;
				break;
			case DW_LNS_negate_stmt:
				is_stmt = !is_stmt;
				break;
			case DW_LNS_set_basic_block:
				break;
			case DW_LNS_const_add_pc:
				address += (255 - opcode_base) / line_range;
				break;
			case DW_LNS_set_prologue_end:
				break;
			case DW_LNS_set_epilogue_begin:
				break;
			default:
				DWARN("unknown standard opcode: %d", opcode);
				return -1;
			}
		}

		if (emit) {
// fprintf(stderr, "ad %llx\n", address);
			if (address > pc) {
				/* Found an entry after our target PC. */
				if (!have_last)
					/* Give up on this program. */
					break;

// fprintf(stderr, "ln %lld\n", last_line);
				/* Return the last entry. */
				*pln = (int)last_line;
				return (read_filename(&names, pdir,
				    pfname, opcode_base, file));
			}

			last_line = line;
			have_last = 1;
		}
	}

	return -1;
}
