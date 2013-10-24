
PROG=	ld
SRCS=	ld.c ld32.c ld64.c syms.c \
	amd64.c arm.c hppa.c i386.c sparc64.c
CLEANFILES+=ld32.c ld64.c
CPPFLAGS+=-I${.CURDIR} -I${.CURDIR}/../nm
CFLAGS+=-Wall -g
LDSTATIC=-static
LDADD=  -lelf
DPADD=  ${LIBELF}

ld32.c: ${.CURDIR}/ld2.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

ld64.c: ${.CURDIR}/ld2.c
	echo '#define ELFSIZE 64' | cat - $> > ${.TARGET}

.if make(install)
#SUBDIR+= PSD.doc
.endif

.include <bsd.prog.mk>
