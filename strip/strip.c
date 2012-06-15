/*
 * Copyright (c) 2009 Michael Shalayeff. All rights reserved.
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char copyright[] =
"Copyright (c) 2009 Michael Shalayeff. All rights reserved.\n"
"@(#) Copyright (c) 1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
/*static char sccsid[] = "from: @(#)strip.c	5.8 (Berkeley) 11/6/91";*/
static char const rcsid[] =
    "$ABSD: strip.c,v 1.9 2012/06/15 16:35:15 mickey Exp $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/exec_elf.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <ranlib.h>
#include <a.out.h>
#include <link_aout.h>
#include <elfuncs.h>

#include "byte.c"

union hdr {
	struct exec aout;
	Elf32_Ehdr elf32;
	Elf64_Ehdr elf64;
};

int s_stab(const char *, struct exec *, struct stat *, off_t *);
int s_sym(const char *, struct exec *, struct stat *, off_t *);
void usage(void);

int xflag = 0;

int
main(int argc, char *argv[])
{
	struct stat sb;
	union hdr eh;
	int (*sfcn)(const char *, struct exec *, struct stat *, off_t *);
	FILE *fp;
	char *fn;
	off_t newsize;
	int ch, rv, errors;

	sfcn = s_sym;
	while ((ch = getopt(argc, argv, "dx")) != -1)
		switch(ch) {
                case 'x':
                        xflag = ELFLIB_STRIPX;
                        /*FALLTHROUGH*/
		case 'd':
                        xflag |= ELFLIB_STRIPD;
			sfcn = s_stab;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	errors = 0;
#define	ERROR(x) errors |= 1; warnx("%s: %s", fn, strerror(x)); continue;
	while ((fn = *argv++)) {
		if (!(fp = fopen(fn, "r+"))) {
			ERROR(errno);
		}
		if (fstat(fileno(fp), &sb)) {
			rv = errno;
			fclose(fp);
			ERROR(rv);
		}
		if (sb.st_size < sizeof(eh)) {
			fclose(fp);
			ERROR(EFTYPE);
		}
		if (fread(&eh, sizeof(eh), 1, fp) != 1) {
			rv = errno;
			fclose(fp);
			ERROR(rv);
		}

		newsize = 0;
		if (!elf32_chk_header(&eh.elf32)) {
			Elf32_Shdr *shdr, *sh, *esh;
			char *shstr;

			elf32_fix_header(&eh.elf32);
			if (eh.elf32.e_ehsize < sizeof eh.elf32)
				errx(1, "%s: ELF header is too short", fn);

			if (sfcn == s_stab)
				errx(1, "not implemented");

			if (!(shdr = elf32_load_shdrs(fn, fp, 0, &eh.elf32)))
				exit(1);

			elf32_fix_shdrs(&eh.elf32, shdr);
			if (!(shstr = elf32_shstrload(fn, fp, 0, &eh.elf32,
			    shdr)))
				exit(1);

			for (sh = shdr, esh = sh + eh.elf32.e_shnum;
			    sh < esh; sh++)
				if (!strcmp(shstr + sh->sh_name, ELF_SYMTAB)) {
					eh.elf32.e_shnum = sh - shdr;
					newsize = eh.elf32.e_shoff +
					    (sh - shdr) * sizeof *shdr;

					if (fseeko(fp, 0, SEEK_SET))
						err(1, "fseeko: %s", fn);

					elf32_fix_header(&eh.elf32);
					if (fwrite(&eh.elf32, sizeof eh.elf32,
					    1, fp) != 1)
						err(1, "fwrite: %s", fn);
				}

		} else if (!elf64_chk_header(&eh.elf64)) {
			Elf64_Shdr *shdr, *sh, *esh;
			char *shstr;

			elf64_fix_header(&eh.elf64);
			if (eh.elf64.e_ehsize < sizeof eh.elf64)
				errx(1, "%s: ELF header is too short", fn);

			if (sfcn == s_stab)
				errx(1, "not implemented");

			if (!(shdr = elf64_load_shdrs(fn, fp, 0, &eh.elf64)))
				exit(1);

			elf64_fix_shdrs(&eh.elf64, shdr);
			if (!(shstr = elf64_shstrload(fn, fp, 0, &eh.elf64,
			    shdr)))
				exit(1);

			for (sh = shdr, esh = sh + eh.elf64.e_shnum;
			    sh < esh; sh++)
				if (!strcmp(shstr + sh->sh_name, ELF_SYMTAB)) {
					eh.elf64.e_shnum = sh - shdr;
					newsize = eh.elf64.e_shoff +
					    (sh - shdr) * sizeof *shdr;

					if (fseeko(fp, 0, SEEK_SET))
						err(1, "fseeko: %s", fn);

					elf64_fix_header(&eh.elf64);
					if (fwrite(&eh.elf64, sizeof eh.elf64,
					    1, fp) != 1)
						err(1, "fwrite: %s", fn);
				}

		} else if (BAD_OBJECT(eh.aout)) {
			fclose(fp);
			ERROR(EFTYPE);
		} else {
			void *ep;
			if ((ep = mmap(NULL, sb.st_size, PROT_READ|PROT_WRITE,
			    MAP_SHARED, fileno(fp), (off_t)0)) == MAP_FAILED) {
				rv = errno;
				fclose(fp);
				ERROR(rv);
			}
			errors |= (sfcn)(fn, ep, &sb, &newsize);
			munmap(ep, sb.st_size);
		}

		/*
		 * since we're dealing with an mmap there,
		 * we have to convert once for dealing with
		 * data in memory, and a second time for out
		 */
		if (fclose(fp)) {
			ERROR(errno);
		}
		if (newsize && truncate(fn, newsize)) {
			ERROR(errno);
		}
	}
#undef ERROR
	exit(errors);
}

int
s_sym(const char *fn, struct exec *ep, struct stat *sp, off_t *sz)
{
	char *neweof;
#if	0
	char *mineof;
#endif
	int zmagic;

	fix_header_order(ep);

	zmagic = ep->a_data &&
		 (N_GETMAGIC(*ep) == ZMAGIC || N_GETMAGIC(*ep) == QMAGIC);

	/*
	 * If no symbols or data/text relocation info and
	 * the file data segment size is already minimized, quit.
	 */
	if (!ep->a_syms && !ep->a_trsize && !ep->a_drsize) {
#if 0
		if (!zmagic)
			return 0;
		if (sp->st_size < N_TRELOFF(*ep))
#endif
			fix_header_order(ep);
			return 0;
	}

	/*
	 * New file size is the header plus text and data segments; OMAGIC
	 * and NMAGIC formats have the text/data immediately following the
	 * header.  ZMAGIC format wastes the rest of of header page.
	 */
	neweof = (char *)ep + N_TRELOFF(*ep);

#if 0
	/*
	 * Unfortunately, this can't work correctly without changing the way
	 * the loader works.  We could cap it at one page, or even fiddle with
	 * a_data and a_bss, but this only works for CLBYTES == NBPG.  If we
	 * are on a system where, e.g., CLBYTES is 8k and NBPG is 4k, and we
	 * happen to remove 4.5k, we will lose.  And we really don't want to
	 * fiddle with pages, because that breaks binary compatibility.  Lose.
	 */

	if (zmagic) {
		/*
		 * Get rid of unneeded zeroes at the end of the data segment
		 * to reduce the file size even more.
		 */
		mineof = (char *)ep + N_DATOFF(*ep);
		while (neweof > mineof && neweof[-1] == '\0')
			neweof--;
	}
#endif

	/* Set symbol size and relocation info values to 0. */
	ep->a_syms = ep->a_trsize = ep->a_drsize = 0;

	/* Truncate the file. */
	*sz = neweof - (char *)ep;

	fix_header_order(ep);
	return 0;
}

int
s_stab(const char *fn, struct exec *ep, struct stat *sp, off_t *sz)
{
	char *nstr, *nstrbase = NULL, *used = NULL, *p, *strbase;
	struct nlist *sym, *nsym, *symbase;
	struct relocation_info *reloc_base;
	unsigned int *mapping = NULL;
	u_long allocsize;
	int cnt, len, mid, nsyms, error = 1;
	u_int i, j;

	fix_header_order(ep);

	/* Quit if no symbols. */
	if (ep->a_syms == 0) {
		fix_header_order(ep);
		return 0;
	}

	if (N_SYMOFF(*ep) >= sp->st_size) {
		warnx("%s: bad symbol table", fn);
		fix_header_order(ep);
		return 1;
	}

	mid = N_GETMID(*ep);

	/*
	 * Initialize old and new symbol pointers.  They both point to the
	 * beginning of the symbol table in memory, since we're deleting
	 * entries.
	 */
	sym = nsym = symbase = (struct nlist *)((char *)ep + N_SYMOFF(*ep));

	/*
	 * Allocate space for the new string table, initialize old and
	 * new string pointers.  Handle the extra long at the beginning
	 * of the string table.
	 */
	strbase = (char *)ep + N_STROFF(*ep);
	allocsize = fix_32_order(*(u_long *)strbase, mid);
	if ((nstrbase = malloc((u_int) allocsize)) == NULL) {
		warnx("%s", strerror(ENOMEM));
		goto end;
	}
	nstr = nstrbase + sizeof(u_long);

	/* okay, so we also need to keep symbol numbers for relocations. */
	nsyms = ep->a_syms/ sizeof(struct nlist);
	used = calloc(nsyms, 1);
	if (!used) {
		warnx("%s", strerror(ENOMEM));
		goto end;
	}
	mapping = calloc(nsyms, sizeof(unsigned int));
	if (!mapping) {
		warnx("%s", strerror(ENOMEM));
		goto end;
	}

	if ((ep->a_trsize || ep->a_drsize) && byte_sex(mid) != BYTE_ORDER) {
		warnx("%s: cross-stripping not supported", fn);
		goto end;
	}

	/* first check the relocations for used symbols, and mark them */
	/* text */
	reloc_base = (struct relocation_info *) ((char *)ep + N_TRELOFF(*ep));
	if (N_TRELOFF(*ep) + ep->a_trsize > sp->st_size) {
		warnx("%s: bad text relocation", fn);
		goto end;
	}
	for (i = 0; i < ep->a_trsize / sizeof(struct relocation_info); i++) {
		if (!reloc_base[i].r_extern)
			continue;
		if (reloc_base[i].r_symbolnum > nsyms) {
			warnx("%s: bad symbol number in text relocation", fn);
			goto end;
		}
		used[reloc_base[i].r_symbolnum] = 1;
	}
	/* data */
	reloc_base = (struct relocation_info *) ((char *)ep + N_DRELOFF(*ep));
	if (N_DRELOFF(*ep) + ep->a_drsize > sp->st_size) {
		warnx("%s: bad data relocation", fn);
		goto end;
	}
	for (i = 0; i < ep->a_drsize / sizeof(struct relocation_info); i++) {
		if (!reloc_base[i].r_extern)
			continue;
		if (reloc_base[i].r_symbolnum > nsyms) {
			warnx("%s: bad symbol number in data relocation", fn);
			goto end;
		}
		used[reloc_base[i].r_symbolnum] = 1;
	}

	/*
	 * Read through the symbol table.  For each non-debugging symbol,
	 * copy it and save its string in the new string table.  Keep
	 * track of the number of symbols.
	 */
	for (cnt = nsyms, i = 0, j = 0; cnt--; ++sym, ++i) {
		fix_nlist_order(sym, mid);
		if (!(sym->n_type & N_STAB) && sym->n_un.n_strx) {
			*nsym = *sym;
			nsym->n_un.n_strx = nstr - nstrbase;
			p = strbase + sym->n_un.n_strx;
                        if (xflag && !used[i] &&
                            (!(sym->n_type & N_EXT) ||
                             (sym->n_type & ~N_EXT) == N_FN ||
                             strcmp(p, "gcc_compiled.") == 0 ||
                             strcmp(p, "gcc2_compiled.") == 0 ||
                             strncmp(p, "___gnu_compiled_", 16) == 0)) {
                                continue;
                        }
			len = strlen(p) + 1;
			mapping[i] = j++;
			if (N_STROFF(*ep)+sym->n_un.n_strx+len > sp->st_size) {
				warnx("%s: bad symbol table", fn);
				goto end;
			}
			bcopy(p, nstr, len);
			nstr += len;
			fix_nlist_order(nsym++, mid);
		}
	}

	/* renumber symbol relocations */
	/* text */
	reloc_base = (struct relocation_info *) ((char *)ep + N_TRELOFF(*ep));
	for (i = 0; i < ep->a_trsize / sizeof(struct relocation_info); i++) {
		if (!reloc_base[i].r_extern)
			continue;
		reloc_base[i].r_symbolnum = mapping[reloc_base[i].r_symbolnum];
	}
	/* data */
	reloc_base = (struct relocation_info *) ((char *)ep + N_DRELOFF(*ep));
	for (i = 0; i < ep->a_drsize / sizeof(struct relocation_info); i++) {
		if (!reloc_base[i].r_extern)
			continue;
		reloc_base[i].r_symbolnum = mapping[reloc_base[i].r_symbolnum];
	}

	/* Fill in new symbol table size. */
	ep->a_syms = (nsym - symbase) * sizeof(struct nlist);

	/* Fill in the new size of the string table. */
	len = nstr - nstrbase;
	*(u_long *)nstrbase = fix_32_order(len, mid);

	/*
	 * Copy the new string table into place.  Nsym should be pointing
	 * at the address past the last symbol entry.
	 */
	bcopy(nstrbase, (void *)nsym, len);
	error = 0;
end:
	fix_header_order(ep);
	free(nstrbase);
	free(used);
	free(mapping);

	/* Truncate to the current length. */
	*sz = (char *)nsym + len - (char *)ep;

	return error;
}

void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s [-dx] file ...\n", __progname);
	exit(1);
}
