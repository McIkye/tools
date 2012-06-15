/*-
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
static char sccsid[] = "@(#)archive.c	8.3 (Berkeley) 4/2/94";
#else
static const char rcsid[] =
    "$ABSD: archive.c,v 1.12 2011/02/07 13:45:41 mickey Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ar.h>
#include <dirent.h>
#include <errno.h>
#include <err.h>

#include "archive.h"

typedef struct ar_hdr HDR;
static char hb[sizeof(HDR) + 1];	/* real header */

FILE *
open_archive(int mode)
{
	FILE *fp;
	int created, fd, nr;
	char buf[SARMAG];

	fd = -1;
	created = 0;
	if (mode & O_CREAT) {
		mode |= O_EXCL;
		if ((fd = open(archive, mode, DEFFILEMODE)) >= 0) {
			/* POSIX.2 puts create message on stderr. */
			if (!(options & AR_C))
				warnx("creating archive %s", archive);
			created = 1;
		} else {
			if (errno != EEXIST)
				err(1, "open: %s", archive);
			mode &= ~O_EXCL;
		}
	}

	if (fd < 0 && (fd = open(archive, mode, DEFFILEMODE)) < 0)
		err(1, "open: %s", archive);

	/*
	 * Attempt to place a lock on the opened file - if we get an
	 * error then someone is already working on this library (or
	 * it's going across NFS).
	 */
	if (flock(fd, LOCK_EX|LOCK_NB) && errno != EOPNOTSUPP)
		err(1, "flock: %s", archive);

	fp = fdopen(fd, mode & O_RDWR? "r+" : "r");

	/*
	 * If not created, O_RDONLY|O_RDWR indicates that it has to be
	 * in archive format.
	 */
	if (!created &&
	    ((mode & O_ACCMODE) == O_RDONLY || (mode & O_ACCMODE) == O_RDWR)) {
		if (fread(buf, SARMAG, 1, fp) != 1)
			err(1, "fread: %s", archive);
		else if (bcmp(buf, ARMAG, SARMAG))
			badfmt();
	} else if (fwrite(ARMAG, SARMAG, 1, fp) != 1)
		err(1, "fwrite: %s", archive);

	return (fp);
}

void
close_archive(FILE *fp)
{
	fclose(fp);			/* Implicit unlock. */
}

/* Convert ar header field to an integer. */
#define	AR_ATOI(from, to, len, base) { \
	memmove(buf, from, len); \
	buf[len] = '\0'; \
	to = strtol(buf, NULL, base); \
}

/*
 * get_arobj --
 *	read the archive header for this member
 */
int
get_arobj(FILE *fp)
{
	struct ar_hdr *hdr;
	int i, len, nr;
	char *p, *q, buf[20];

	nr = fread(hb, sizeof(HDR), 1, fp);
	if (nr != 1) {
		if (feof(fp))
			return (0);
		if (ferror(fp))
			err(1, "fread: %s", archive);
		badfmt();
	}

	hdr = (struct ar_hdr *)hb;
	if (strncmp(hdr->ar_fmag, ARFMAG, sizeof(ARFMAG) - 1))
		badfmt();

	/* Convert the header into the internal format. */
#define	DECIMAL	10
#define	OCTAL	 8

	AR_ATOI(hdr->ar_date, chdr.date, sizeof(hdr->ar_date), DECIMAL);
	AR_ATOI(hdr->ar_uid, chdr.uid, sizeof(hdr->ar_uid), DECIMAL);
	AR_ATOI(hdr->ar_gid, chdr.gid, sizeof(hdr->ar_gid), DECIMAL);
	AR_ATOI(hdr->ar_mode, chdr.mode, sizeof(hdr->ar_mode), OCTAL);
	AR_ATOI(hdr->ar_size, chdr.size, sizeof(hdr->ar_size), DECIMAL);

	/* Leading spaces should never happen. */
	if (hdr->ar_name[0] == ' ')
		badfmt();

	/*
	 * Long name support.  Set the "real" size of the file, and the
	 * long name flag/size.
	 */
	if (!bcmp(hdr->ar_name, AR_EFMT1, sizeof(AR_EFMT1) - 1)) {
		chdr.lname = len = atoi(hdr->ar_name + sizeof(AR_EFMT1) - 1);
		if (len <= 0 || len > MAXNAMLEN)
			badfmt();
		if (fread(chdr.name, len, 1, fp) != 1) {
			if (ferror(fp))
				err(1, "fread: %s", archive);
			badfmt();
		}
		chdr.name[len] = 0;
		chdr.size -= len;
	} else if (hdr->ar_name[0] == '/' && isdigit(hdr->ar_name[1])) {
		i = atol(&hdr->ar_name[1]);
		/* XXX check overflow */
		chdr.lname = 0;
		for (p = chdr.name, q = &nametab[i]; *q != '/'; )
			*p++ = *q++;
		*p = '\0';
	} else {
		chdr.lname = 0;
		memmove(chdr.name, hdr->ar_name, sizeof(hdr->ar_name));

		/* Strip trailing spaces, null terminate. */
		for (p = chdr.name + sizeof(hdr->ar_name) - 1; *p == ' '; --p);
		if (p > chdr.name + 1 && *p == '/')
			p--;
		p[1] = '\0';
	}

	return (1);
}

static int already_written;

/*
 * put_arobj --
 *	Write an archive member to a file.
 */
void
put_arobj(CF *cfp, struct stat *sb)
{
	off_t size;
	const char *name;
	struct ar_hdr *hdr;
	int lname;
	uid_t uid;
	gid_t gid;

	/*
	 * If passed an sb structure, reading a file from disk.  Get stat(2)
	 * information, build a name and construct a header.  (Files are named
	 * by their last component in the archive.)  If not, then just write
	 * the last header read.
	 */
	if (sb) {
		name = rname(cfp->rname);
		(void)fstat(fileno(cfp->rfp), sb);

		/*
		 * If not truncating names and the name is too long or contains
		 * a space, use extended format 1 or 2 (.
		 */
		lname = strlen(name);
		uid = sb->st_uid;
		gid = sb->st_gid;
		if (uid > USHRT_MAX) {
			warnx("warning: uid %u truncated to %u", uid,
			    USHRT_MAX);
			uid = USHRT_MAX;
		}
		if (gid > USHRT_MAX) {
			warnx("warning: gid %u truncated to %u", gid,
			    USHRT_MAX);
			gid = USHRT_MAX;
		}
		if (options & AR_TR) {
			if (lname > OLDARMAXNAME) {
				(void)fflush(stdout);
				warnx("warning: file name %s truncated to %.*s",
				    name, OLDARMAXNAME, name);
				(void)fflush(stderr);
			}
			(void)snprintf(hb, sizeof hb,
			    HDR3, name, (long int)sb->st_mtimespec.tv_sec,
			    uid, gid, sb->st_mode, sb->st_size, ARFMAG);
			lname = 0;
		} else if (lname >= sizeof(hdr->ar_name) || strchr(name, ' ')) {
			if (options & AR_S5) {
				char *p = strstr(nametab, name);

				if (!p || p[lname] != '/' ||
				    (p != nametab && p[-1] != '\n'))
					errx(1, "corrupt nametab");

				(void)snprintf(hb, sizeof hb, HDR0, p - nametab,
				    (long)sb->st_mtimespec.tv_sec, uid, gid,
				    sb->st_mode, sb->st_size, ARFMAG);

				/* prevent BSD4.4 name prependition */
				lname = 0;
			} else
				(void)snprintf(hb, sizeof hb, HDR1, AR_EFMT1,
				    lname, (long)sb->st_mtimespec.tv_sec,
				    uid, gid, sb->st_mode,
				    sb->st_size + lname, ARFMAG);
		} else {
			lname = 0;
			(void)snprintf(hb, sizeof hb,
			    HDR2, name, (long int)sb->st_mtimespec.tv_sec,
			    uid, gid, sb->st_mode, sb->st_size, ARFMAG);
		}
		size = sb->st_size;
	} else {
		lname = chdr.lname;
		name = chdr.name;
		size = chdr.size;
	}

	if (fwrite(hb, sizeof(HDR), 1, cfp->wfp) != 1)
		err(1, "fwrite: %s", cfp->wname);
	if (lname) {
		if (fwrite(name, lname, 1, cfp->wfp) != 1)
			err(1, "fwrite: %s", cfp->wname);
		already_written = lname;
	}
	copy_ar(cfp, size);
	already_written = 0;
}

int
get_namtab(FILE *afp)
{
	if (!(nametab = malloc(chdr.size + 1)))
		err(1, "%s: alloc nametab", archive);

	if (fread(nametab, chdr.size, 1, afp) != 1)
		err(1, "%s: read nametab", archive);
	nametab[chdr.size] = '\0';
	if (nametab[chdr.size - 1] == '\n')
		nametab[chdr.size - 1] = '\0';
}

int
put_nametab(CF *cfp)
{
	size_t sz;
	char pad = 0;

	if (!nametab)
		return 0;

	if ((sz = strlen(nametab)) & 1) {
		sz++;
		pad = '\n';
	}
	(void)snprintf(hb, sizeof hb,
	    HDR4, AR_NAMTAB, "", "", "", "", (quad_t)sz, ARFMAG);

	if (fwrite(hb, sizeof(HDR), 1, cfp->wfp) != 1)
		err(1, "write: %s", cfp->wname);

	if (pad)
		sz--;
	if (fwrite(nametab, sz, 1, cfp->wfp) != 1)
		err(1, "write: %s", cfp->wname);

	if (pad && fwrite(&pad, 1, 1, cfp->wfp) != 1)
		err(1, "fwrite: %s", cfp->wname);

	return 0;
}

/*
 * copy_ar --
 *	Copy size bytes from one file to another - taking care to handle the
 *	extra byte (for odd size files) when reading archives and writing an
 *	extra byte if necessary when adding files to archive.  The length of
 *	the object is the long name plus the object itself; the variable
 *	already_written gets set if a long name was written.
 */
void
copy_ar(CF *cfp, off_t size)
{
	char buf[MAXBSIZE], pad = 0;
	off_t sz;
	int nr, nw, off;

	if (!(sz = size))
		return;

	while (sz && (nr = fread(buf, 1, MIN(sz, sizeof buf), cfp->rfp)) > 0) {
		sz -= nr;
		for (off = 0; off < nr; nr -= off, off += nw)
			if ((nw = fwrite(buf + off, 1, nr, cfp->wfp)) <= 0)
				err(1, "fwrite: %s", cfp->wname);
	}
	if (sz) {
		if (nr == 0)
			badfmt();
		err(1, "fread: %s", cfp->rname);
	}
	if ((size & 1)) {
		if (fwrite(&pad, 1, 1, cfp->wfp) <= 0)
			err(1, "fwrite: %s", cfp->wname);
		if (fseek(cfp->rfp, 1, SEEK_CUR) < 0)
			err(1, "fseek: %s", cfp->rname);
	}
}

/*
 * skip_arobj -
 *	Skip over an object -- taking care to skip the pad bytes.
 */
off_t
skip_arobj(FILE *fp)
{
	int pad;

	pad = chdr.size & 1;
	if (fseeko(fp, chdr.size + pad, SEEK_CUR) == -1)
		err(1, "fseeko: %s", archive);

	return chdr.size + pad + sizeof(struct ar_hdr);
}
