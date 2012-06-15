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
static char sccsid[] = "@(#)misc.c	8.3 (Berkeley) 4/2/94";
#else
static const char rcsid[] =
    "$ABSD: misc.c,v 1.4 2009/05/26 20:39:07 mickey Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>

#include "archive.h"
#include "pathnames.h"

const char tname[] = "temporary file";	/* temporary file "name" */

FILE *
tmp(void)
{
	extern char *envtmp;
	char path[MAXPATHLEN];
	FILE *fp;
	sigset_t set, oset;
	int fd;

	if (!envtmp && !(envtmp = getenv("TMPDIR")))
		envtmp = _PATH_TMP;

	if (envtmp)
		(void)snprintf(path, sizeof(path), "%s/%s", envtmp,
		    _NAME_ARTMP);
	else
		strlcpy(path, _PATH_ARTMP, sizeof(path));

	sigfillset(&set);
	(void)sigprocmask(SIG_BLOCK, &set, &oset);
	if ((fd = mkstemp(path)) == -1)
		err(1, "mkstemp: %s", tname);
        (void)unlink(path);
	(void)sigprocmask(SIG_SETMASK, &oset, NULL);

	if (!(fp = fdopen(fd, "r+")))
		err(1, "fdopen: %s", tname);
	return (fp);
}

/*
 * files --
 *	See if the current file matches any file in the argument list; if it
 *	does, remove it from the argument list.
 */
char *
files(char **argv)
{
	char **list, *p;

	for (list = argv; *list; ++list)
		if (compare(*list)) {
			p = *list;
			for (; (list[0] = list[1]); ++list)
				continue;
			return (p);
		}
	return (NULL);
}

void
orphans(char **argv)
{

	for (; *argv; ++argv)
		warnx("%s: not found in archive", *argv);
}

const char *
rname(const char *path)
{
	const char *ind;

	return ((ind = strrchr(path, '/')) ? ind + 1 : path);
}

int
compare(const char *dest)
{

	if (options & AR_TR)
		return (!strncmp(chdr.name, rname(dest), OLDARMAXNAME));
	return (!strcmp(chdr.name, rname(dest)));
}

void
badfmt(void)
{

	errno = EFTYPE;
	err(1, "%s", archive);
}
