
PROG=	readelf
SRCS=	readelf.c readelf32.c readelf64.c
CPPFLAGS+=-I${.CURDIR}
CFLAGS+=-Wall -g
LDSTATIC=-static
DPADD=	${LIBELF}
LDADD=	-lelf

readelf32.c: ${.CURDIR}/readelf2.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

readelf64.c: ${.CURDIR}/readelf2.c
	echo '#define ELFSIZE 64' | cat - $> > ${.TARGET}

.include <bsd.prog.mk>
