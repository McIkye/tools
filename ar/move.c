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
static char sccsid[] = "@(#)move.c	8.3 (Berkeley) 4/2/94";
#else
static const char rcsid[] =
    "$ABSD: move.c,v 1.4 2009/05/26 20:39:07 mickey Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <ar.h>
#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "archive.h"
#include "pathnames.h"

/*
 * move --
 *	Change location of named members in archive - if 'b' or 'i' option
 *	selected then named members are placed before 'posname'.  If 'a'
 *	option selected members go after 'posname'.  If no options, members
 *	are moved to end of archive.
 */
int
move(char **argv)
{
	CF cf;
	off_t size, tsize;
	FILE *afp, *curfp, *tfp1, *tfp2, *tfp3;
	char *file;
	int mods;

	afp = open_archive(O_RDWR);
	mods = options & (AR_A|AR_B);

	tfp1 = tmp();			/* Files before key file. */
	tfp2 = tmp();			/* Files selected by user. */
	tfp3 = tmp();			/* Files after key file. */

	/*
	 * Break archive into three parts -- selected entries and entries
	 * before and after the key entry.  If positioning before the key,
	 * place the key at the beginning of the after key entries and if
	 * positioning after the key, place the key at the end of the before
	 * key entries.  Put it all back together at the end.
	 */

	/* Read and write to an archive; pad on both. */
	SETCF(afp, archive, 0, tname, 0);
	for (curfp = tfp1; get_arobj(afp);) {
		if (*argv && (file = files(argv))) {
			if (options & AR_V)
				(void)printf("m - %s\n", file);
			cf.wfp = tfp2;
			put_arobj(&cf, NULL);
			continue;
		}
		if (mods && compare(posname)) {
			mods = 0;
			if (options & AR_B)
				curfp = tfp3;
			cf.wfp = curfp;
			put_arobj(&cf, NULL);
			if (options & AR_A)
				curfp = tfp3;
		} else {
			cf.wfp = curfp;
			put_arobj(&cf, NULL);
		}
	}

	if (mods) {
		warnx("%s: archive member not found", posarg);
		close_archive(afp);
		return (1);
	}
	(void)fseeko(afp, SARMAG, SEEK_SET);

	SETCF(tfp1, tname, afp, archive, NOPAD);
	tsize = size = ftello(tfp1);
	rewind(tfp1);
	copy_ar(&cf, size);

	tsize += size = ftello(tfp2);
	rewind(tfp2);
	cf.rfp = tfp2;
	copy_ar(&cf, size);

	tsize += size = ftello(tfp3);
	rewind(tfp3);
	cf.rfp = tfp3;
	copy_ar(&cf, size);

	fflush(afp);
	(void)ftruncate(fileno(afp), tsize + SARMAG);
	close_archive(afp);

	if (*argv) {
		orphans(argv);
		return (1);
	}
	return (0);
}
