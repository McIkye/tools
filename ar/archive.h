/*-
 * Copyright (c) 1991, 1993, 1994
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
 *
 *	@(#)archive.h	8.3 (Berkeley) 4/2/94
 *	@(#)extern.h	8.3 (Berkeley) 4/2/94
 */

/* Ar(1) options. */
#define	AR_A	0x00001
#define	AR_B	0x00002
#define	AR_C	0x00004
#define	AR_D	0x00008
#define	AR_M	0x00010
#define	AR_O	0x00020
#define	AR_P	0x00040
#define	AR_Q	0x00080
#define	AR_R	0x00100
#define	AR_S	0x00200
#define	AR_T	0x00400
#define	AR_TR	0x00800
#define	AR_U	0x01000
#define	AR_V	0x02000
#define	AR_X	0x04000
#define	AR_CC	0x08000
#define	AR_S5	0x10000
extern u_int options;

/* Set up file copy. */
#define	SETCF(from, fromname, to, toname, pad) { \
	cf.rfp = from; \
	cf.rname = fromname; \
	cf.wfp = to; \
	cf.wname = toname; \
}

/* File copy structure. */
typedef struct {
	FILE	*rfp;			/* read file descriptor */
	const char *rname;		/* read name */
	FILE	*wfp;			/* write file descriptor */
	const char *wname;		/* write name */
} CF;

/* Header structure internal format. */
typedef struct {
	off_t size;			/* size of the object in bytes */
	time_t date;			/* date */
	int lname;			/* size of the long name in bytes */
	gid_t gid;			/* group */
	uid_t uid;			/* owner */
	u_short mode;			/* permissions */
	char name[MAXNAMLEN + 1];	/* name */
} CHDR;

/* Header format strings. */
#define	HDR0	"/%-15td%-12ld%-6u%-6u%-8o%-10qd%2s"
#define	HDR1	"%s/%-13d%-12ld%-6u%-6u%-8o%-10qd%2s"
#define	HDR2	"%-16.16s%-12ld%-6u%-6u%-8o%-10qd%2s"

#define	OLDARMAXNAME	15
#define	HDR3	"%-16.15s%-12ld%-6u%-6u%-8o%-10qd%2s"
#define	HDR4	"%-16.16s%-12s%-6s%-6s%-8s%-10qd%2s"

struct stat;

FILE	*open_archive(int);
void	close_archive(FILE *);
void	copy_ar(CF *, off_t);
int	get_arobj(FILE *);
void	put_arobj(CF *, struct stat *);
int	get_namtab(FILE *);
int	put_nametab(CF *);
off_t	skip_arobj(FILE *);

int	append(char **);
void	badfmt(void);
int	compare(const char *);
int	contents(char **);
int	delete(char **);
int	extract(char **);
char	*files(char **argv);
int	move(char **);
void	orphans(char **argv);
int	print(char **);
int	replace(char **);
const char *rname(const char *);
FILE	*tmp(void);
int	ranlib(char **);

extern const char *archive;
extern const char *posarg, *posname;	/* positioning file name */
extern const char tname[];		/* temporary file "name" */
extern CHDR chdr;			/* converted header */
extern char *nametab;			/* long name table */
