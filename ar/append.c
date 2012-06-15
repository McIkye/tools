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
static char sccsid[] = "@(#)append.c	8.3 (Berkeley) 4/2/94";
#else
static const char rcsid[] =
    "$ABSD: append.c,v 1.7 2011/02/03 23:24:38 mickey Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ar.h>
#include <err.h>

#include "archive.h"

int mknametab(char **);

char *nametab;

/*
 * append --
 *	Append files to the archive - modifies original archive or creates
 *	a new archive if named archive does not exist.
 */
int
append(char **argv)
{
	struct stat sb;
	CF cf;
	FILE *afp;
	char *file;
	int eval, ntsz;

	afp = open_archive(O_CREAT|O_RDWR);
	if (fseeko(afp, 0, SEEK_END) == (off_t)-1)
		err(1, "lseek: %s", archive);

	/* Read from disk, write to an archive; pad on write. */
	SETCF(0, 0, afp, archive, 0);

	if (options & AR_C) {
		/* ain't no need for a.out but a small price to pay */
		if ((ntsz = mknametab(argv)) == 0)
			;

		/* build and write the symdef, also need a mid! */
		/* mkranlib(argv);
		if (mid & ELF) */ {
			options |= AR_S5;
			/* write out the nametab */
			if (ntsz)
				put_nametab(&cf);
		}
	}

	for (eval = 0; (file = *argv++);) {
		if (!(cf.rfp = fopen(file, "r"))) {
			warn("fopen: %s", file);
			eval = 1;
			continue;
		}
		if (options & AR_V)
			(void)printf("q - %s\n", file);
		cf.rname = file;
		put_arobj(&cf, &sb);
		(void)fclose(cf.rfp);
	}
	close_archive(afp);
	return (eval);
}

int
mknametab(char **argv)
{
	struct ar_hdr *hdr;
	char **av, *p, *q;
	int i, s, t;

	/* first count the needed space */
	for (t = 0, av = argv; *av; av++) {
		q = strrchr(*av, '/');
		q = q? q + 1 : *av;
		if ((s = strlen(q)) >= sizeof(hdr->ar_name))
			t += s + 2;
	}

	if (!t)
		return 0;

	if (!(nametab = malloc(t + 1)))
		err(1, "malloc: nametab");

	/* and now populate */
	for (i = 0, av = argv, p = nametab; *av; av++) {
		q = strrchr(*av, '/');
		q = q? q + 1 : *av;
		if ((s = strlen(q)) >= sizeof(hdr->ar_name)) {
			snprintf(p, s + 3, "%s/\n", q);
			p += s + 2;
		}
	}
	*p = '\0';

	return t;
}
