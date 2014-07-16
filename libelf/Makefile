
LIB=	elf
SRCS=	elf32.c elf32_header.c elf32_shdrs.c elf32_symload.c elf32_size.c \
	elf64.c elf64_header.c elf64_shdrs.c elf64_symload.c elf64_size.c \
	checkoff.c
CFLAGS+=-Wall
CLEANFILES+=elf32.c elf32_header.c elf32_shdrs.c elf32_symload.c elf32_size.c
CLEANFILES+=elf64.c elf64_header.c elf64_shdrs.c elf64_symload.c elf64_size.c
MAN=	elf_size.3
MLINKS=	elf_size.3 elf_fix_header.3 elf_size.3 elf_chk_header.3 \
	elf_size.3 elf_fix_note.3 elf_size.3 elf_save_phdrs.3 \
	elf_size.3 elf_load_shdrs.3 elf_size.3 elf_save_shdrs.3 \
	elf_size.3 elf_fix_shdrs.3 elf_size.3 elf_shstrload.3 \
	elf_size.3 elf_load_phdrs.3 elf_size.3 elf_fix_phdrs.3 \
	elf_size.3 elf_strload.3 elf_size.3 elf_symload.3 \
	elf_size.3 elf_fix_sym.3 elf_size.3 elf2nlist.3 \
	elf_size.3 elf_fix_rel.3 elf_size.3 elf_fix_rela.3

elf32.c: ${.CURDIR}/elf.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

elf64.c: ${.CURDIR}/elf.c
	echo '#define ELFSIZE 64' | cat - $> > ${.TARGET}

elf32_header.c: ${.CURDIR}/elf_header.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

elf64_header.c: ${.CURDIR}/elf_header.c
	echo '#define ELFSIZE 64' | cat - $> > ${.TARGET}

elf32_shdrs.c: ${.CURDIR}/elf_shdrs.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

elf64_shdrs.c: ${.CURDIR}/elf_shdrs.c
	echo '#define ELFSIZE 64' | cat - $> > ${.TARGET}

elf32_symload.c: ${.CURDIR}/elf_symload.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

elf64_symload.c: ${.CURDIR}/elf_symload.c
	echo '#define ELFSIZE 64' | cat - $> > ${.TARGET}

elf32_size.c: ${.CURDIR}/elf_size.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

elf64_size.c: ${.CURDIR}/elf_size.c
	echo '#define ELFSIZE 64' | cat - $> > ${.TARGET}

includes:
	@cmp -s ${DESTDIR}/usr/include/elfuncs.h ${.CURDIR}/elfuncs.h || \
	${INSTALL} ${INSTALL_COPY} -m 444 -o $(BINOWN) -g $(BINGRP) \
	    ${.CURDIR}/elfuncs.h ${DESTDIR}/usr/include/elfuncs.h

.include <bsd.lib.mk>
