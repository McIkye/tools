/*
 * Copyright (c) 2009 Michael Shalayeff. All rights reserved.
 * Copyright (c) 1980, 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
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
"@(#) Copyright (c) 1980, 1987, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static const char sccsid[] = "@(#)strings.c	8.2 (Berkeley) 1/28/94";
#endif
static const char rcsid[] =
    "$ABSD: strings.c,v 1.7 2012/06/15 16:35:15 mickey Exp $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/exec_elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>
#include <errno.h>
#include <ranlib.h>
#include <err.h>
#include <a.out.h>
#include <elfuncs.h>

/* XXX get shared code to handle a.out byte-order swaps */
#include "byte.c"

#define FORMAT_DEC "%07ld "
#define FORMAT_OCT "%07lo "
#define FORMAT_HEX "%07lx "

#define DEF_LEN		4	/* default minimum string length */
#define ISSTR(ch)	(isascii(ch) && (isprint(ch) || ch == '\t'))
#define	SZXHEAD		128	/* XXX shall be enough */
#define	MAXMAXLEN	65536

long	foff;			/* offset in the file */
int	hcnt,			/* head count */
	head_len,		/* length of header */
	read_len;		/* length to read */
u_char	hbfr[SZXHEAD];		/* buffer for struct exec */
u_char *bfr;
int asdata, fflg, minlen, maxlen;
char *offset_format;

void usage(void);
int getch(void);
int strings(const char *);
int strscan(const char *);

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int ch, exitcode, bfrlen;
	const char *file, *p, *errstr;

	setlocale(LC_ALL, "");

	/*
	 * for backward compatibility, allow '-' to specify 'a' flag; no
	 * longer documented in the man page or usage string.
	 */
	exitcode = 0;
	minlen = -1;
	maxlen = -1;
	while ((ch = getopt(argc, argv, "0123456789an:m:oft:-")) != -1)
		switch((char)ch) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			/*
			 * kludge: strings was originally designed to take
			 * a number after a dash.
			 */
			if (minlen == -1) {
				p = argv[optind - 1];
				if (p[0] == '-' && p[1] == ch && !p[2])
					minlen = atoi(++p);
				else
					minlen = atoi(argv[optind] + 1);
			}
			break;
		case '-':
		case 'a':
			asdata = 1;
			break;
		case 'f':
			fflg = 1;
			break;
		case 'n':
			minlen = strtonum(optarg, 1, MAXMAXLEN, &errstr);
			if (errstr)
				err(1, "illegal minlen \"%s\"", optarg);
			break;
		case 'm':
			maxlen = strtonum(optarg, 1, MAXMAXLEN, &errstr);
			if (errstr)
				err(1, "illegal maxlen \"%s\"", optarg);
			break;
		case 'o':
			offset_format = FORMAT_OCT;
			break;
		case 't':
			switch (*optarg) {
			case 'o':
			        offset_format = FORMAT_OCT;
				break;
			case 'd':
				offset_format = FORMAT_DEC;
				break;
			case 'x':
				offset_format = FORMAT_HEX;
				break;
			default:
				usage();
				/* NOTREACHED */
			}
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (minlen == -1)
		minlen = DEF_LEN;
	else if (minlen < 1)
		errx(1, "length less than 1");
	if (maxlen != -1 && maxlen < minlen)
		errx(1, "max length less than min");
	bfrlen = maxlen == -1 ? minlen : maxlen;
	bfr = malloc(bfrlen + 1);
	if (!bfr)
		err(1, "malloc");
	bfr[bfrlen] = '\0';
	file = "stdin";
	do {
		if (*argv) {
			file = *argv++;
			if (!freopen(file, "r", stdin)) {
				warn("%s", file);
				exitcode = 1;
				continue;
			}
		}
		exitcode |= strings(file);
	} while (*argv);

	exit(exitcode);
}

int
strings(const char *file)
{
	Elf32_Ehdr *elf32 = (Elf32_Ehdr *)hbfr;
	Elf64_Ehdr *elf64 = (Elf64_Ehdr *)hbfr;
	int rv;

	rv = 0;
	foff = 0;
	read_len = -1;
	head_len = 0;
	if (!asdata) {
		if ((head_len = read(fileno(stdin), hbfr, SZXHEAD)) != SZXHEAD)
			head_len = 0;
		else if (!elf32_chk_header(elf32)) {
			Elf32_Phdr *phdr;
			int i;

			elf32_fix_header(elf32);
			if (elf32->e_ehsize < sizeof *elf32) {
				warnx("%s: ELF header is too short", file);
				return 1;
			}

			if ((phdr = elf32_load_phdrs(file, stdin, 0, elf32))) {
				elf32_fix_phdrs(elf32, phdr);
				for (i = elf32->e_phnum; i--; phdr++) {
					if (phdr->p_type != PT_LOAD ||
					    phdr->p_memsz == 0 ||
					    phdr->p_filesz == 0)
						continue;
					foff = phdr->p_offset;
					if (fseek(stdin, foff, SEEK_SET) == -1)
						head_len = 0;
					else {
						read_len = phdr->p_filesz;
						head_len = 0;
					}
					rv |= strscan(file);
				}
				return rv;
			} else
				hcnt = 0;

		} else if (!elf64_chk_header(elf64)) {
			Elf64_Phdr *phdr;
			int i;

			elf64_fix_header(elf64);
			if (elf64->e_ehsize < sizeof *elf64) {
				warnx("%s: ELF header is too short", file);
				return 1;
			}

			if ((phdr = elf64_load_phdrs(file, stdin, 0, elf64))) {
				elf64_fix_phdrs(elf64, phdr);
				for (i = elf64->e_phnum; i--; phdr++) {
					if (phdr->p_type != PT_LOAD ||
					    phdr->p_memsz == 0 ||
					    phdr->p_filesz == 0)
						continue;
					foff = phdr->p_offset;
					if (fseek(stdin, foff, SEEK_SET) == -1)
						head_len = 0;
					else {
						read_len = phdr->p_filesz;
						head_len = 0;
					}
					rv |= strscan(file);
				}
				return rv;
			} else
				hcnt = 0;

		} else if (!BAD_OBJECT(*(struct exec *)hbfr)) {
			struct exec *head;

			head = (struct exec *)hbfr;
			fix_header_order(head);
			foff = N_TXTOFF(*head);
			if (fseek(stdin, foff, SEEK_SET) == -1)
				head_len = 0;
			else {
				read_len = head->a_text + head->a_data;
				head_len = 0;
			}
			return strscan(file);
		} else
			hcnt = 0;
	}
	return strscan(file);
}

int
strscan(const char *file)
{
	u_char *C;
	int ch, cnt;

	for (cnt = 0, C = bfr; (ch = getch()) != EOF;) {
		if (ISSTR(ch)) {
			*C++ = ch;
			if (++cnt < minlen)
				continue;
			if (maxlen != -1) {
				while ((ch = getch()) != EOF &&
				       ISSTR(ch) && cnt++ < maxlen)
					*C++ = ch;
				if (ch == EOF ||
				    (ch != 0 && ch != '\n')) {
					/* get all of too big string */
					while ((ch = getch()) != EOF &&
					       ISSTR(ch))
						;
					ungetc(ch, stdin);
					goto out;
				}
				*C = 0;
			}

			if (fflg)
				printf("%s:", file);

			if (offset_format) 
				printf(offset_format, foff - minlen);

			printf("%s", bfr);

			if (maxlen == -1)
				while ((ch = getch()) != EOF &&
				    ISSTR(ch))
					putchar((char)ch);
			putchar('\n');
		out:
			;
		}
		cnt = 0;
		C = bfr;
	}

	return (0);
}

/*
 * getch --
 *	get next character from wherever
 */
int
getch(void)
{
	++foff;
	if (head_len) {
		if (hcnt < head_len)
			return((int)hbfr[hcnt++]);
		head_len = 0;
	}
	if (read_len == -1 || read_len-- > 0)
		return(getchar());
	return(EOF);
}

void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: strings [-afo] [-m number] [-n number] [-t radix] [file ...]\n");
	exit(1);
}
