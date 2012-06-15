#	@(#)bsd.doc.mk	8.1 (Berkeley) 8/14/93

BIB?=		bib
EQN?=		eqn
NEQN?=		neqn
GREMLIN?=	grn
GRIND?=		vgrind -f
INDXBIB?=	indxbib
PIC?=		pic
REFER?=		refer
ROFF?=		groff -M/usr/share/tmac ${MACROS} ${PAGES}
SOELIM?=	soelim
TBL?=		tbl

.PATH: ${.CURDIR}

.if !target(all)
.MAIN: all
all: paper.ps
.endif

.if !target(paper.ps)
paper.ps: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
.endif

.if !target(print)
print: paper.ps
	lpr -P${PRINTER} paper.ps
.endif

.if !target(manpages)
manpages:
.endif

.if !target(obj)
obj:
.endif

clean cleandir:
	rm -f paper.* [eE]rrs mklog ${CLEANFILES}

FILES?=	${SRCS}
install:
	${INSTALL} ${INSTALL_COPY} -o ${DOCOWN} -g ${DOCGRP} -m ${DOCMODE} \
	    Makefile ${FILES} ${EXTRA} ${DESTDIR}${DOCDIR}/${DIR}

spell: paper.spell


paper.spell: ${SRCS}
	spell ${SRCS} | sort | comm -23 - spell.ok > paper.spell

.include <bsd.own.mk>
