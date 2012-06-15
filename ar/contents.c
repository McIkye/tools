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
static char sccsid[] = "@(#)contents.c	8.3 (Berkeley) 4/2/94";
#else
static const char rcsid[] =
    "$ABSD: contents.c,v 1.4 2009/05/26 20:39:07 mickey Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <ar.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <tzfile.h>
#include <unistd.h>

#include "archive.h"

/*
 * contents --
 *	Handles t[v] option - opens the archive and then reads headers,
 *	skipping member contents.
 */
int
contents(char **argv)
{
	off_t size;
	FILE *afp;
	struct tm *tp;
	char *file, buf[25];
	int all;

	afp = open_archive(O_RDONLY);
	for (all = !*argv; get_arobj(afp); skip_arobj(afp)) {
		if (!strncmp(chdr.name, AR_NAMTAB, sizeof(AR_NAMTAB) - 1)) {
			size = ftello(afp);
			get_namtab(afp);
			(void)fseeko(afp, size, SEEK_SET);
			continue;
		}

		if (all)
			file = chdr.name;
		else if (!(file = files(argv)))
			continue;
		if (options & AR_V) {
			(void)strmode(chdr.mode, buf);
			(void)printf("%s %6d/%-6d %8qd ",
			    buf + 1, chdr.uid, chdr.gid, chdr.size);
			tp = localtime(&chdr.date);
			(void)strftime(buf, sizeof(buf), "%b %e %H:%M %Y", tp);
			(void)printf("%s %s\n", buf, file);
		} else
			(void)printf("%s\n", file);
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
