
PROG=	ar
CPPFLAGS+=-I${.CURDIR} -I${.CURDIR}/../nm
SRCS=	append.c ar.c archive.c contents.c delete.c extract.c misc.c \
	move.c print.c ranlib.c replace.c
MAN=	ar.1 ar.5 ranlib.1 ranlib.5
LINKS=	${BINDIR}/ar ${BINDIR}/ranlib
LDADD=	-lelf
DPADD=	${LIBELF}

.include <bsd.prog.mk>
