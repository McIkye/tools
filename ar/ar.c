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
static const char copyright[] =
"@(#) Copyright (c) 2009 Michael Shalayeff.  All rights reserved.\n"
"@(#) Copyright (c) 1990, 1993, 1994\n"
    "\tThe Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)ar.c	8.3 (Berkeley) 4/2/94";
#else
static const char rcsid[] =
    "$ABSD: ar.c,v 1.8 2010/06/18 11:24:36 mickey Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>
#include <ar.h>
#include <dirent.h>
#include <err.h>

#include "archive.h"

CHDR chdr;
u_int options;

const char *archive, *envtmp, *posarg;
const char *posname;
void badoptions(char *);
void usage(void);

/*
 * main --
 *	main basically uses getopt to parse options and calls the appropriate
 *	functions.  Some hacks that let us be backward compatible with 4.3 ar
 *	option parsing and sanity checking.
 */
int
main(int argc, char *argv[])
{
	extern char *__progname;
	int c;
	char *p;
	int (*fcall)(char **);

	fcall = NULL;
	if (strcmp(__progname, "ranlib") == 0) {
		if (argc < 2)
			usage();

		options |= AR_S;
		while ((c = getopt(argc, argv, "t")) != -1)
			switch(c) {
			case 't':
				options |= AR_T;
				break;
			default:
				usage();
			}
	} else {
		if (argc < 3)
			usage();

		/*
		 * Historic versions didn't require a '-' in front of the
		 * options.  Fix it, if necessary.
		 */
		if (*argv[1] != '-') {
			size_t len;

			len = (u_int)(strlen(argv[1]) + 2);
			if (!(p = malloc(len)))
				err(1, NULL);
			*p = '-';
			(void)strlcpy(p + 1, argv[1], len - 1);
			argv[1] = p;
		}
		while ((c = getopt(argc, argv, "abcCdilmopqrsTtuvx")) != -1)
			switch(c) {
			case 'a':
				options |= AR_A;
				break;
			case 'b':
			case 'i':
				options |= AR_B;
				break;
			case 'c':
				options |= AR_C;
				break;
			case 'C':
				options |= AR_CC;
				break;
			case 'd':
				options |= AR_D;
				fcall = delete;
				break;
			case 'l':
				/*
				 * "local" tmp-files option;
				 * ignored for compatibility
				 */
				break;
			case 'm':
				options |= AR_M;
				fcall = move;
				break;
			case 'o':
				options |= AR_O;
				break;
			case 'p':
				options |= AR_P;
				fcall = print;
				break;
			case 'q':
				/* TODO optimise symdef generation for -c */
				options |= AR_Q;
				fcall = append;
				break;
			case 'r':
				options |= AR_R;
				fcall = replace;
				break;
			case 's':
				options |= AR_S;
				break;
			case 'T':
				options |= AR_TR;
				break;
			case 't':
				options |= AR_T;
				fcall = contents;
				break;
			case 'u':
				options |= AR_U;
				break;
			case 'v':
				options |= AR_V;
				break;
			case 'x':
				options |= AR_X;
				fcall = extract;
				break;
			default:
				usage();
			}
	}

	argv += optind;
	argc -= optind;

	/* single -s is a ranlib for the ar files given */
	if (fcall == NULL && (options & AR_S))
		exit(ranlib(argv));

	/* One of -dmpqrtx required. */
	if (!(options & (AR_D|AR_M|AR_P|AR_Q|AR_R|AR_T|AR_X))) {
		warnx("one of options -dmpqrtx is required");
		usage();
	}
	/* Only one of -a and -bi allowed. */
	if (options & AR_A && options & AR_B) {
		warnx("only one of -a and -[bi] options allowed");
		usage();
	}
	/* -ab require a position argument. */
	if (options & (AR_A|AR_B)) {
		if (!(posarg = *argv++)) {
			warnx("no position operand specified");
			usage();
		}
		posname = rname(posarg);
	}
	/* -d only valid with -sTv. */
	if (options & AR_D && options & ~(AR_D|AR_TR|AR_S|AR_V))
		badoptions("-d");
	/* -m only valid with -abiTsv. */
	if (options & AR_M && options & ~(AR_A|AR_B|AR_M|AR_TR|AR_S|AR_V))
		badoptions("-m");
	/* -p only valid with -Tv. */
	if (options & AR_P && options & ~(AR_P|AR_TR|AR_V))
		badoptions("-p");
	/* -q only valid with -csTv. */
	if (options & AR_Q && options & ~(AR_C|AR_Q|AR_TR|AR_S|AR_V))
		badoptions("-q");
	/* -r only valid with -abcuTsv. */
	if (options & AR_R &&
	    options & ~(AR_A|AR_B|AR_C|AR_R|AR_U|AR_TR|AR_S|AR_V))
		badoptions("-r");
	/* -t only valid with -Tv. */
	if (options & AR_T && options & ~(AR_T|AR_TR|AR_V))
		badoptions("-t");
	/* -x only valid with -CouTv. */
	if (options & AR_X && options & ~(AR_O|AR_U|AR_TR|AR_V|AR_X|AR_CC))
		badoptions("-x");

	if (!(archive = *argv++)) {
		warnx("no archive specified");
		usage();
	}

	exit((*fcall)(argv));
}

void
badoptions(char *arg)
{

	warnx("illegal option combination for %s", arg);
	usage();
}

void
usage(void)
{

	fprintf(stderr, "usage:  ar -d [-sTv] archive file ...\n"
	    "\tar -m [-sTv] archive file ...\n"
	    "\tar -m [-abisTv] position archive file ...\n"
	    "\tar -p [-Tv] archive [file ...]\n"
	    "\tar -q [-csTv] archive file ...\n"
	    "\tar -r [-cusTv] archive file ...\n"
	    "\tar -r [-abciusTv] position archive file ...\n"
	    "\tar -t [-Tv] archive [file ...]\n"
	    "\tar -x [-CouTv] archive [file ...]\n"
	    "\tranlib [-t] archive ...\n");
	exit(1);
}
