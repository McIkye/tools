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
static char sccsid[] = "@(#)extract.c	8.3 (Berkeley) 4/2/94";
#else
static const char rcsid[] =
    "$ABSD: extract.c,v 1.5 2011/01/31 13:33:06 mickey Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ar.h>

#include "archive.h"

/*
 * extract --
 *	Extract files from the named archive - if member names given only
 *	extract those members otherwise extract all members.  If 'o' option
 *	selected modify date of newly created file to be same as archive
 *	members date otherwise date is time of extraction.  Does not modify
 *	archive.
 */
int
extract(char **argv)
{
	struct timeval tv[2];
	struct stat sb;
	CF cf;
	off_t size;
	FILE *afp;
	char *file;
	int all, eval, tfd;

	eval = 0;
	tv[0].tv_usec = tv[1].tv_usec = 0;

	afp = open_archive(O_RDONLY);

	/* Read from an archive, write to disk; pad on read. */
	SETCF(afp, archive, 0, 0, 0);
	for (all = !*argv; get_arobj(afp);) {
		if (!strncmp(chdr.name, AR_NAMTAB, sizeof(AR_NAMTAB) - 1)) {
			size = ftello(afp);
			get_namtab(afp);
			(void)fseeko(afp, size, SEEK_SET);
			skip_arobj(afp);
			continue;
		}

		if (all)
			file = chdr.name;
		else if (!(file = files(argv))) {
			skip_arobj(afp);
			continue;
		}

		if (options & AR_U && !stat(file, &sb) &&
		    sb.st_mtime > chdr.date)
			continue;

		if (options & AR_CC) {
			/* -C means do not overwrite existing files */
			if ((tfd = open(file, O_WRONLY|O_CREAT|O_EXCL,
			    S_IWUSR)) < 0) {
				skip_arobj(afp);
				eval = 1;
				continue;
			}
		} else {
			if ((tfd = open(file, O_WRONLY|O_CREAT|O_TRUNC,
			    S_IWUSR)) < 0) {
				warn("open: %s", file);
				skip_arobj(afp);
				eval = 1;
				continue;
			}
		}

		if (options & AR_V)
			(void)printf("x - %s\n", file);

		if (!(cf.wfp = fdopen(tfd, "w")))
			err(1, "fdopen: %s", file);
		cf.wname = file;
		copy_ar(&cf, chdr.size);

		if (fchmod(tfd, (mode_t)chdr.mode)) {
			warn("chmod: %s", file);
			eval = 1;
		}
		if (options & AR_O) {
			tv[0].tv_sec = tv[1].tv_sec = chdr.date;
			if (futimes(tfd, tv)) {
				warn("utimes: %s", file);
				eval = 1;
			}
		}
		(void)fclose(cf.wfp);
		if (!all && !*argv)
			break;
	}
	close_archive(afp);

	if (*argv) {
		orphans(argv);
		return (1);
	}
	return (0);
}
