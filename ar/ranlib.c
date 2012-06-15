/*-
 * Copyright (c) 2009 Michael Shalayeff
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Hugh Smith at The University of Guelph.
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
#if 0
static char sccsid[] = "@(#)build.c	5.3 (Berkeley) 3/12/91";
static char sccsid[] = "@(#)touch.c	5.3 (Berkeley) 3/12/91";
#else
static const char rcsid[] = "$ABSD: ranlib.c,v 1.7 2012/06/15 16:35:15 mickey Exp $";
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <dirent.h>
#include <a.out.h>
#include <ar.h>
#include <ranlib.h>
#include <elf_abi.h>
#include <elfuncs.h>

#include "archive.h"

/* from usr.bin/nm/ */
#include "byte.c"

#define	MID_ELFFL	0x80000000	/* signal for the symdef format */

extern CHDR chdr;			/* converted header */

typedef struct _rlib {
	struct _rlib *next;		/* next structure */
	off_t pos;			/* offset of defining archive file */
	char *sym;			/* symbol */
	int symlen;			/* strlen(sym) */
} RLIB;
RLIB *rhead, **pnext;
off_t r_fuzz;
int ntsize;

int read_exec(FILE *, FILE *, long *, long *);
void symobj(FILE *, int, long, long);
int touch(void);
void settime(FILE *);
int do_ranlib(void);
int addsym(struct nlist *, const char *, off_t, long *, long *, const char *);

int
ranlib(char **argv)
{
	int rv;

	archive = *argv;
	do {
		rv = options & AR_T? touch() : do_ranlib();
		if (!rv && *argv)
			archive = *argv;
	} while (rv && *argv++);

	return rv;
}

int
do_ranlib(void)
{
	CF cf;
	off_t size;
	FILE	*afp, *tfp;
	long	symcnt;			/* symbol count */
	long	tsymlen;		/* total string length */
	int	i, current_mid;

	current_mid = -1;
	afp = open_archive(O_RDWR);
	tfp = tmp();

	SETCF(afp, archive, tfp, tname, 0);

	/* Read through the archive, creating list of symbols. */
	symcnt = tsymlen = 0;
	ntsize = 0;
	r_fuzz = SARMAG;
	pnext = &rhead;
	while(get_arobj(afp)) {
		int new_mid;

		if (!strncmp(chdr.name, AR_NAMTAB, sizeof(AR_NAMTAB) - 1)) {
			size = ftello(afp);
			get_namtab(afp);
			ntsize = chdr.size + sizeof(struct ar_hdr);
			(void)fseeko(afp, size, SEEK_SET);
			r_fuzz += skip_arobj(afp);
			continue;
		}

		if (!strncmp(chdr.name, RANLIBMAG, sizeof(RANLIBMAG) - 1) ||
		    !strncmp(chdr.name, RANLIBMAG2, sizeof(RANLIBMAG2) - 1)) {
			r_fuzz += skip_arobj(afp);
			continue;
		}
		new_mid = read_exec(afp, tfp, &symcnt, &tsymlen);
		if (new_mid != -1) {
			if (current_mid == -1)
				current_mid = new_mid;
			else if (new_mid != current_mid)
				errx(1, "mixed object format archive: %x / %x",
					new_mid, current_mid);
		}
		put_arobj(&cf, NULL);
	}
	*pnext = NULL;

	SETCF(tfp, tname, afp, archive, NOPAD);

	/* Create the symbol table.  Endianess the same as last mid seen */
	symobj(afp, current_mid, symcnt, tsymlen);
	put_nametab(&cf);

	/* Copy the saved objects into the archive. */
	size = ftello(tfp);
	rewind(tfp);
	copy_ar(&cf, size);
	fflush(afp);
	(void)ftruncate(fileno(afp), ftello(afp));
	(void)fclose(tfp);

	/* Set the time. */
	settime(afp);
	close_archive(afp);
	return(0);
}

/*
 * read_exec
 *	Read the exec structure; ignore any files that don't look
 *	exactly right. Return MID.
 *	return -1 for files that don't look right.
 *	XXX it's hard to be sure when to ignore files, and when to error
 *	out.
 */
int
read_exec(FILE *rfp, FILE *wfp, long *symcnt, long *tsymlen)
{
	union {
		struct exec exec;
		Elf32_Ehdr elf32;
		Elf64_Ehdr elf64;
	} eh;
	struct nlist nl;
	off_t r_off, w_off;
	char *strtab = NULL;
	long strsize, nsyms;
	int i;

	/* Get current offsets for original and tmp files. */
	r_off = ftello(rfp);
	w_off = ftello(wfp);

	/* Read in exec structure. */
	if (fread(&eh, sizeof(eh), 1, rfp) != 1)
		err(1, "fread: %s", archive);

	if (!elf32_chk_header(&eh.elf32)) {
		Elf32_Sym sbuf;
		char *shstr;
		Elf32_Shdr *shdr;
		size_t stabsize;

		elf32_fix_header(&eh.elf32);
		if (eh.elf32.e_ehsize < sizeof eh.elf32) {
			warnx("%s: ELF header is too short", archive);
			goto bad;
		}

		if (!(shdr = elf32_load_shdrs(archive, rfp, r_off, &eh.elf32)))
			goto bad;
		elf32_fix_shdrs(&eh.elf32, shdr);

		if (!(shstr = elf32_shstrload(archive, rfp, r_off, &eh.elf32,
		    shdr))) {
			free(shdr);
			goto bad;
		}

		if (!(strtab = elf32_strload(archive, rfp, r_off, &eh.elf32,
		    shdr, shstr, ELF_STRTAB, &stabsize))) {
			free(shstr);
			free(shdr);
			goto bad;
		}

		/* find the symtab section */
		for (i = 0; i < eh.elf32.e_shnum; i++)
			if (!strcmp(shstr + shdr[i].sh_name, ELF_SYMTAB)) {
				nsyms = shdr[i].sh_size / sizeof(Elf32_Sym);
				break;
			}

		if (i == eh.elf32.e_shnum) {
			free(shstr);
			free(shdr);
			goto bad;
		}

		if (fseeko(rfp, r_off + shdr[i].sh_offset, SEEK_SET))
			err(1, "fseeko: %s", archive);

		for (i = 0; i < nsyms; i++) {
			if (fread(&sbuf, sizeof(sbuf), 1, rfp) != 1)
				err(1, "fread: %s", archive);

			elf32_fix_sym(&eh.elf32, &sbuf);
			if (!sbuf.st_name || sbuf.st_name > stabsize)
				continue;

			if (elf32_2nlist(&sbuf, &eh.elf32, shdr, shstr, &nl))
				continue;

			addsym(&nl, strtab, r_off - r_fuzz -
			    sizeof(struct ar_hdr), symcnt, tsymlen, archive);
		}

		free(strtab);
		free(shstr);
		free(shdr);
		(void)fseeko(rfp, r_off, SEEK_SET);
		return MID_ELFFL | eh.elf32.e_machine;

	} else if (!elf64_chk_header(&eh.elf64)) {
		Elf64_Sym sbuf;
		char *shstr;
		Elf64_Shdr *shdr;
		size_t stabsize;

		elf64_fix_header(&eh.elf64);
		if (eh.elf64.e_ehsize < sizeof eh.elf64) {
			warnx("%s: ELF header is too short", archive);
			goto bad;
		}

		if (!(shdr = elf64_load_shdrs(archive, rfp, r_off, &eh.elf64)))
			goto bad;
		elf64_fix_shdrs(&eh.elf64, shdr);

		if (!(shstr = elf64_shstrload(archive, rfp, r_off, &eh.elf64,
		    shdr))) {
			free(shdr);
			goto bad;
		}

		if (!(strtab = elf64_strload(archive, rfp, r_off, &eh.elf64,
		    shdr, shstr, ELF_STRTAB, &stabsize))) {
			free(shstr);
			free(shdr);
			goto bad;
		}

		/* find the symtab section */
		for (i = 0; i < eh.elf64.e_shnum; i++)
			if (!strcmp(shstr + shdr[i].sh_name, ELF_SYMTAB)) {
				nsyms = shdr[i].sh_size / sizeof(Elf64_Sym);
				break;
			}

		if (i == eh.elf64.e_shnum) {
			free(shstr);
			free(shdr);
			goto bad;
		}

		if (fseeko(rfp, r_off + shdr[i].sh_offset, SEEK_SET))
			err(1, "fseeko: %s", archive);

		for (i = 0; i < nsyms; i++) {
			if (fread(&sbuf, sizeof(sbuf), 1, rfp) != 1)
				err(1, "fread: %s", archive);

			elf64_fix_sym(&eh.elf64, &sbuf);
			if (!sbuf.st_name || sbuf.st_name > stabsize)
				continue;

			if (elf64_2nlist(&sbuf, &eh.elf64, shdr, shstr, &nl))
				continue;

			addsym(&nl, strtab, r_off - r_fuzz -
			    sizeof(struct ar_hdr), symcnt, tsymlen, archive);
		}

		free(strtab);
		free(shstr);
		free(shdr);
		(void)fseeko(rfp, r_off, SEEK_SET);
		return MID_ELFFL | eh.elf64.e_machine;

	} else if (BAD_OBJECT(eh.exec) || eh.exec.a_syms == 0)
		goto bad;

	fix_header_order(&eh.exec);

	/* Seek to string table. */
	if (fseeko(rfp, N_STROFF(eh.exec) + r_off, SEEK_SET) == -1) {
		if (errno == EINVAL)
			goto bad;
		else
			err(1, "lseek: %s", archive);
	}

	/* Read in size of the string table. */
	if (fread((char *)&strsize, sizeof(strsize), 1, rfp) != 1)
		err(1, "fread: %s", archive);

	strsize = fix_32_order(strsize, N_GETMID(eh.exec));

	/* Read in the string table. */
	strsize -= sizeof(strsize);
	strtab = malloc(strsize);
	if (!strtab)
		err(1, "malloc: %s", archive);
	if (fread(strtab, strsize, 1, rfp) != 1)
		err(1, "fread: %s", archive);

	/* Seek to symbol table. */
	if (fseek(rfp, N_SYMOFF(eh.exec) + r_off, SEEK_SET) == (off_t)-1)
		err(1, "fseeko: %s", archive);

	/* For each symbol read the nlist entry and save it as necessary. */
	nsyms = eh.exec.a_syms / sizeof(struct nlist);
	while (nsyms--) {
		if (!fread((char *)&nl, sizeof(struct nlist), 1, rfp)) {
			if (feof(rfp))
				badfmt();
			err(1, "fread: %s", archive);
		}
		fix_nlist_order(&nl, N_GETMID(eh.exec));

		addsym(&nl, strtab - sizeof(long), r_off - r_fuzz -
		    sizeof(struct ar_hdr), symcnt, tsymlen, archive);
	}

bad:
	free(strtab);
	(void)fseeko(rfp, r_off, SEEK_SET);
	return N_GETMID(eh.exec);
}

int
addsym(struct nlist *nl, const char *strtab, off_t off, long *symcnt,
    long *tsymlen, const char *name)
{
	const char *sym;
	RLIB *rp;
	size_t symlen;

	/* Ignore if no name or local. */
	if (!nl->n_un.n_strx || !(nl->n_type & N_EXT) ||
	    (nl->n_type & N_TYPE) == N_FN)
		return 1;

	/*
	 * If the symbol is an undefined external and the n_value
	 * field is non-zero, keep it.
	 */
	if ((nl->n_type & N_TYPE) == N_UNDF && !nl->n_value)
		return 1;

	/* First four bytes are the table size. */
	sym = strtab + nl->n_un.n_strx;
	symlen = strlen(sym) + 1;

	if (!(rp = malloc(sizeof(RLIB))) || !(rp->sym = malloc(symlen)))
		err(1, "malloc: %s", name);
	bcopy(sym, rp->sym, symlen);
	rp->symlen = symlen;
	rp->pos = off;

	/* Build in forward order for "ar m" command. */
	*pnext = rp;
	pnext = &rp->next;

	++*symcnt;
	*tsymlen += symlen;

	return 0;
}

/*
 * symobj --
 *	Write the symbol table into the archive, computing offsets as
 *	writing.  Use the right format depending on mid.
 */
void
symobj(FILE *fp, int mid, long symcnt, long tsymlen)
{
	RLIB *rp, *rnext;
	struct ranlib rn;
	char hb[sizeof(struct ar_hdr) + 1], pad;
	int pos, ransize, size, stroff;
	uid_t uid;
	gid_t gid;

	/* Rewind the archive, leaving the magic number and nametab */
	if (fseek(fp, SARMAG, SEEK_SET) == (off_t)-1)
		err(1, "fseek: %s", archive);

	/* Size of the ranlib archive file, pad if necessary. */
	if (mid & MID_ELFFL)
		ransize = sizeof(int) +
		    symcnt * sizeof(int) + tsymlen;
	else
		ransize = sizeof(int) +
		    symcnt * sizeof(struct ranlib) + sizeof(int) + tsymlen;
	if (ransize & 01) {
		++ransize;
		pad = '\n';
	} else
		pad = '\0';

	uid = getuid();
	if (uid > USHRT_MAX) {
		warnx("warning: uid %u truncated to %u", uid, USHRT_MAX);
		uid = USHRT_MAX;
	}
	gid = getgid();
	if (gid > USHRT_MAX) {
		warnx("warning: gid %u truncated to %u", gid, USHRT_MAX);
		gid = USHRT_MAX;
	}

	/* Put out the ranlib archive file header. */
#define	DEFMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
	(void)snprintf(hb, sizeof hb, HDR2,
	    mid & MID_ELFFL? RANLIBMAG2 : RANLIBMAG, 0L, uid, gid,
	    DEFMODE & ~umask(0), (off_t)ransize, ARFMAG);
	if (!fwrite(hb, sizeof(struct ar_hdr), 1, fp))
		err(1, "fwrite: %s", tname);

	/* First long is the size of the ranlib structure section. */
	if (mid & MID_ELFFL)
		size = htonl(symcnt);
	else
		size = fix_32_order(symcnt * sizeof(struct ranlib), mid);
	if (!fwrite((char *)&size, sizeof(size), 1, fp))
		err(1, "fwrite: %s", tname);

	/* Offset of the first archive file. */
	size = SARMAG + sizeof(struct ar_hdr) + ransize + ntsize;

	/*
	 * Write out the ranlib structures.  The offset into the string
	 * table is cumulative, the offset into the archive is the value
	 * set in read_exec() plus the offset to the first archive file.
	 */
	for (rp = rhead, stroff = 0; rp; rp = rp->next) {
		if (mid & MID_ELFFL) {
			pos = htonl(size + rp->pos);
			if (fwrite(&pos, sizeof(pos), 1, fp) != 1)
				err(1, "fwrite: %s", archive);
		} else {
			rn.ran_un.ran_strx = stroff;
			stroff += rp->symlen;
			rn.ran_off = size + rp->pos;
			fix_ranlib_order(&rn, mid);
			if (fwrite(&rn, sizeof(struct ranlib), 1, fp) != 1)
				err(1, "fwrite: %s", archive);
		}
	}

	/* Second long is the size of the string table. */

	if (!(mid & MID_ELFFL)) {
		size = fix_32_order(tsymlen, mid);
		if (fwrite(&size, sizeof(size), 1, fp) != 1)
			err(1, "fwrite: %s", tname);
	}

	/* Write out the string table. */
	for (rp = rhead; rp; rp = rnext) {
		if (!fwrite(rp->sym, rp->symlen, 1, fp))
			err(1, "fwrite: %s", tname);
		rnext = rp->next;
		free(rp);
	}
	rhead = NULL;

	if (pad && !fwrite(&pad, sizeof(pad), 1, fp))
		err(1, "fwrite: %s", tname);

	(void)fflush(fp);
}

int
touch(void)
{
	FILE *afp;

	afp = open_archive(O_RDWR);

	if (!get_arobj(afp) ||
	    (strncmp(RANLIBMAG, chdr.name, sizeof(RANLIBMAG) - 1) &&
	     strncmp(RANLIBMAG2, chdr.name, sizeof(RANLIBMAG2) - 1))) {
		warnx("%s: no symbol table.", archive);
		return(1);
	}
	settime(afp);
	close_archive(afp);
	return(0);
}

void
settime(FILE *afp)
{
	struct ar_hdr *hdr;
	off_t size;
	char buf[50];

	size = SARMAG + sizeof(hdr->ar_name);
	if (fseeko(afp, size, SEEK_SET) == -1)
		err(1, "fseeko: %s", archive);
	(void)snprintf(buf, sizeof buf,
	    "%-12ld", (long)time((time_t *)NULL) + RANLIBSKEW);
	if (fwrite(buf, sizeof(hdr->ar_date), 1, afp) != 1)
		err(1, "write: %s", archive);
}
