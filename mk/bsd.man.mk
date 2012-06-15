#	@(#)bsd.man.mk	5.2 (Berkeley) 5/11/90

MANTARGET?=	cat
NROFF?=		nroff -Tascii
MANHTMLOFF?=	awk -f ${BSDSRCDIR}/usr.bin/manhtml/manhtml.awk
TBL?=		tbl
EQN?=		eqn
NEQN?=		neqn
MANLINT?=	\#

.if !target(.MAIN)
.  if exists(${.CURDIR}/../Makefile.inc)
.    include "${.CURDIR}/../Makefile.inc"
.  endif

.MAIN: all
.endif

.SUFFIXES: .1 .2 .3 .3p .4 .5 .6 .7 .8 .9 \
	.1eqn .2eqn .3eqn .4eqn .5eqn .6eqn .7eqn .8eqn .9eqn \
	.1tbl .2tbl .3tbl .4tbl .5tbl .6tbl .7tbl .8tbl .9tbl \
	.cat1 .cat2 .cat3 .cat3p .cat4 .cat5 .cat6 .cat7 .cat8 .cat9 \
	.html1 .html2 .html3 .html3p .html4 .html5 .html6 .html7 .html8 .html9 \
	.ps1 .ps2 .ps3 .ps3p .ps4 .ps5 .ps6 .ps7 .ps8 .ps9

.9.cat9 .8.cat8 .7.cat7 .6.cat6 .5.cat5 .4.cat4 .3p.cat3p .3.cat3 .2.cat2 .1.cat1:
	@echo "${NROFF} -mandoc ${.IMPSRC} > ${.TARGET}"
	@${MANLINT} ${.IMPSRC}
	@${NROFF} -mandoc ${.IMPSRC} > ${.TARGET} || (rm -f ${.TARGET}; false)

.9tbl.cat9 .8tbl.cat8 .7tbl.cat7 .6tbl.cat6 .5tbl.cat5 .4tbl.cat4 .3tbl.cat3 \
.2tbl.cat2 .1tbl.cat1:
	@echo "${TBL} ${.IMPSRC} | ${NROFF} -mandoc > ${.TARGET}"
	@${MANLINT} -tbl ${.IMPSRC}
	@${TBL} ${.IMPSRC} | ${NROFF} -mandoc > ${.TARGET} || \
	    (rm -f ${.TARGET}; false)

.9eqn.cat9 .8eqn.cat8 .7eqn.cat7 .6eqn.cat6 .5eqn.cat5 .4eqn.cat4 .3eqn.cat3 \
.2eqn.cat2 .1eqn.cat1:
	@echo "${NEQN} ${.IMPSRC} | ${NROFF} -mandoc > ${.TARGET}"
	@${MANLINT} -eqn ${.IMPSRC}
	@${NEQN} ${.IMPSRC} | ${NROFF} -mandoc > ${.TARGET} || \
	    (rm -f ${.TARGET}; false)

.9.ps9 .8.ps8 .7.ps7 .6.ps6 .5.ps5 .4.ps4 .3p.ps3p .3.ps3 .2.ps2 .1.ps1:
	@echo "nroff -Tps -mandoc ${.IMPSRC} > ${.TARGET}"
	@nroff -Tps -mandoc ${.IMPSRC} > ${.TARGET} || (rm -f ${.TARGET}; false)

.9tbl.ps9 .8tbl.ps8 .7tbl.ps7 .6tbl.ps6 .5tbl.ps5 .4tbl.ps4 .3tbl.ps3 \
.2tbl.ps2 .1tbl.ps1:
	@echo "${TBL} ${.IMPSRC} | nroff -Tps -mandoc > ${.TARGET}"
	@${TBL} ${.IMPSRC} | nroff -Tps -mandoc > ${.TARGET} || \
	    (rm -f ${.TARGET}; false)

.9eqn.ps9 .8eqn.ps8 .7eqn.ps7 .6eqn.ps6 .5eqn.ps5 .4eqn.ps4 .3eqn.ps3 \
.2eqn.ps2 .1eqn.ps1:
	@echo "${EQN} ${.IMPSRC} | nroff -Tps -mandoc > ${.TARGET}"
	@${EQN} ${.IMPSRC} | nroff -Tps -mandoc > ${.TARGET} || \
	    (rm -f ${.TARGET}; false)

.9.html9 .8.html8 .7.html7 .6.html6 .5.html5 .4.html4 .3p.html3p .3.html3 \
.2.html2 .1.html1:
	@echo "manhtml ${.IMPSRC} > ${.TARGET}"
	@${MANHTMLOFF} -v OSNAME=`uname -s` -v OSREL=${OSREV} \
	    ${.IMPSRC} > ${.TARGET} || (rm -f ${.TARGET}; false)

.9tbl.html9 .8tbl.html8 .7tbl.html7 .6tbl.html6 .5tbl.html5 .4tbl.html4 \
.3tbl.html3 .2tbl.html2 .1tbl.html1:
	@echo "${TBL} ${.IMPSRC} | manhtml > ${.TARGET}"
	@${TBL} ${.IMPSRC} | ${MANHTMLOFF} -v OSNAME=`uname -s` \
	    -v OSREL=${OSREV} > ${.TARGET} || (rm -f ${.TARGET}; false)

.9eqn.html9 .8eqn.html8 .7eqn.html7 .6eqn.html6 .5eqn.html5 .4eqn.html4 \
.3eqn.html3 .2eqn.html2 .1eqn.html1:
	@echo "${EQN} ${.IMPSRC} | manhtml > ${.TARGET}"
	@${EQN} ${.IMPSRC} | ${MANHTMLOFF} -v OSNAME=`uname -s` \
	    -v OSREL=${OSREV} > ${.TARGET} || (rm -f ${.TARGET}; false)

.if defined(MAN) && !empty(MAN) && !defined(MANALL)
.  for v s in MANALL .cat PS2ALL .ps HTML2ALL .html

$v=	${MAN:S/.1$/$s1/g:S/.2$/$s2/g:S/.3$/$s3/g:S/.3p$/$s3p/g:S/.4$/$s4/g:S/.5$/$s5/g:S/.6$/$s6/g:S/.7$/$s7/g:S/.8$/$s8/g:S/.9$/$s9/g:S/.1tbl$/$s1/g:S/.2tbl$/$s2/g:S/.3tbl$/$s3/g:S/.4tbl$/$s4/g:S/.5tbl$/$s5/g:S/.6tbl$/$s6/g:S/.7tbl$/$s7/g:S/.8tbl$/$s8/g:S/.9tbl$/$s9/g:S/.1eqn$/$s1/g:S/.2eqn$/$s2/g:S/.3eqn$/$s3/g:S/.4eqn$/$s4/g:S/.5eqn$/$s5/g:S/.6eqn$/$s6/g:S/.7eqn$/$s7/g:S/.8eqn$/$s8/g:S/.9eqn$/$s9/g}

.  endfor

.  if defined(MANPS)
PSALL=${PS2ALL}
.  endif

.  if defined(MANHTML)
HTMLALL=${HTML2ALL}
.  endif

.endif

MINSTALL=	${INSTALL} ${INSTALL_COPY} -o ${MANOWN} -g ${MANGRP} -m ${MANMODE}
.if defined(MANZ)
# chown and chmod are done afterward automatically
MCOMPRESS=	gzip -cf
MCOMPRESSSUFFIX= .gz
.endif

.if defined(MANSUBDIR)
# Add / so that we don't have to specify it. Better arch -> MANSUBDIR mapping
MANSUBDIR:=${MANSUBDIR:S,^,/,}
.else
# XXX MANSUBDIR must be non empty for the mlink loops to work
MANSUBDIR=''
.endif

.if !defined(MCOMPRESS) || empty(MCOMPRESS)
install_manpage_fragment= \
	echo ${MINSTALL} $$page $$instpage; \
	${MINSTALL} $$page $$instpage
.else
install_manpage_fragment= \
	rm -f $$instpage; \
	echo ${MCOMPRESS} $$page \> $$instpage; \
	${MCOMPRESS} $$page > $$instpage; \
	chown ${MANOWN}:${MANGRP} $$instpage; \
	chmod ${MANMODE} $$instpage
.endif

maninstall:
.for v d s t in MANALL ${MANDIR} .cat .0 PSALL ${PSDIR} .ps .ps \
    HTMLALL ${HTMLDIR} .html .html
.  if defined($v)
	@for page in ${$v}; do \
		set -- ${MANSUBDIR}; \
		subdir=$$1; \
		dir=${DESTDIR}$d$${page##*$s}; \
		base=$${page##*/}; \
		instpage=$${dir}$${subdir}/$${base%.*}$t${MCOMPRESSSUFFIX}; \
		${install_manpage_fragment}; \
		while test $$# -ge 2; do \
			shift; \
			extra=$${dir}$$1/$${base%.*}$t${MCOMPRESSSUFFIX}; \
			echo $$extra -\> $$instpage; \
			ln -f $$instpage $$extra; \
		done; \
	done
.  endif
.endfor

.if defined(MLINKS) && !empty(MLINKS)
.  for sub in ${MANSUBDIR}
.     for lnk file in ${MLINKS}
	@l=${DESTDIR}${MANDIR}${lnk:E}${sub}/${lnk:R}.0${MCOMPRESSSUFFIX}; \
	t=${DESTDIR}${MANDIR}${file:E}${sub}/${file:R}.0${MCOMPRESSSUFFIX}; \
	echo $$t -\> $$l; \
	rm -f $$t; ln $$l $$t;
.       if defined(MANPS)
	  @l=${DESTDIR}${PSDIR}${lnk:E}${sub}/${lnk:R}.ps; \
	  t=${DESTDIR}${PSDIR}${file:E}${sub}/${file:R}.ps; \
	  echo $$t -\> $$l; \
	  rm -f $$t; ln $$l $$t;
.       endif
.       if defined(MANHTML)
	  @l=${DESTDIR}${HTMLDIR}${lnk:E}${sub}/${lnk:R}.html; \
	  t=${DESTDIR}${HTMLDIR}${file:E}${sub}/${file:R}.html; \
	  echo $$t -\> $$l; \
	  rm -f $$t; ln $$l $$t;
.       endif
.     endfor
.  endfor
.endif

.if (defined(MANALL) || defined(PSALL) || defined(HTMLALL)) && \
    !defined(MANLOCALBUILD)
all: ${MANALL} ${PSALL} ${HTMLALL}

BEFOREMAN?=
${MANALL} ${PSALL} ${HTMLALL}: ${BEFOREMAN}

cleandir: cleanman
cleanman:
	rm -f ${MANALL} ${PS2ALL} ${HTML2ALL} ${CLEANFILES}
.endif
