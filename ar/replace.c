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
static char sccsid[] = "@(#)replace.c	8.3 (Berkeley) 4/2/94";
#else
static const char rcsid[] = "$ABSD: replace.c,v 1.5 2009/07/30 12:16:13 mickey Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ar.h>
#include <dirent.h>
#include <errno.h>
#include <err.h>

#include "archive.h"

/*
 * replace --
 *	Replace or add named members to archive.  Entries already in the
 *	archive are swapped in place.  Others are added before or after
 *	the key entry, based on the a, b and i options.  If the u option
 *	is specified, modification dates select for replacement.
 */
int
replace(char **argv)
{
	struct stat sb;
	CF cf;
	off_t size, tsize;
	FILE *afp, *sfp, *curfp, *tfp1, *tfp2;
	char *file;
	int errflg, mods;

	errflg = 0;
	/*
	 * If doesn't exist, simply append to the archive.  There's
	 * a race here, but it's pretty short, and not worth fixing.
	 */
	if (stat(archive, &sb)) {
		if (errno == ENOENT)
			append(argv);
		else
			err(1, "stat: %s", archive);
	}

	/* TODO update nametab */

	afp = open_archive(O_CREAT|O_RDWR);

	tfp1 = tmp();			/* Files before key file. */
	tfp2 = tmp();			/* Files after key file. */

	/*
	 * Break archive into two parts -- entries before and after the key
	 * entry.  If positioning before the key, place the key at the
	 * beginning of the after key entries and if positioning after the
	 * key, place the key at the end of the before key entries.  Put it
	 * all back together at the end.
	 */
	mods = (options & (AR_A|AR_B));
	for (curfp = tfp1; get_arobj(afp);) {
		if (*argv && (file = files(argv))) {
			if (!(sfp = fopen(file, "r"))) {
				errflg = 1;
				warn("%s", file);
				goto useold;
			}
			(void)fstat(fileno(sfp), &sb);
			if (options & AR_U && sb.st_mtime <= chdr.date) {
				fclose(sfp);
				goto useold;
			}

			if (options & AR_V)
			     (void)printf("r - %s\n", file);

			/* Read from disk, write to an archive; pad on write */
			SETCF(sfp, file, curfp, tname, 0);
			put_arobj(&cf, &sb);
			(void)fclose(sfp);
			skip_arobj(afp);
			continue;
		}

		if (mods && compare(posname)) {
			mods = 0;
			if (options & AR_B)
				curfp = tfp2;
			/* Read and write to an archive; pad on both. */
			SETCF(afp, archive, curfp, tname, 0);
			put_arobj(&cf, NULL);
			if (options & AR_A)
				curfp = tfp2;
		} else {
			/* Read and write to an archive; pad on both. */
useold:			SETCF(afp, archive, curfp, tname, 0);
			put_arobj(&cf, NULL);
		}
	}

	if (mods) {
		warnx("%s: archive member not found", posarg);
                close_archive(afp);
                return (1);
        }

	/* Append any left-over arguments to the end of the after file. */
	while ((file = *argv++)) {
		if (options & AR_V)
			(void)printf("a - %s\n", file);
		if (!(sfp = fopen(file, "r"))) {
			errflg = 1;
			warn("%s", file);
			continue;
		}
		(void)fstat(fileno(sfp), &sb);
		/* Read from disk, write to an archive; pad on write. */
		SETCF(sfp, file,
		    options & (AR_A|AR_B) ? tfp1 : tfp2, tname, 0);
		put_arobj(&cf, &sb);
		(void)fclose(sfp);
	}

	(void)fseeko(afp, SARMAG, SEEK_SET);

	SETCF(tfp1, tname, afp, archive, 0);
	if (tfp1) {
		tsize = size = ftello(tfp1);
		rewind(tfp1);
		copy_ar(&cf, size);
	} else
		tsize = 0;

	tsize += size = ftello(tfp2);
	rewind(tfp2);
	cf.rfp = tfp2;
	copy_ar(&cf, size);

	fflush(afp);
	(void)ftruncate(fileno(afp), tsize + SARMAG);
	close_archive(afp);
	return (errflg);
}
