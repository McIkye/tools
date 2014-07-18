
LIB=	elf
SRCS=	elf32_header.c elf32_phdrs.c elf32_shdrs.c elf32_rel.c \
	elf32_symload.c elf32_sym.c elf32_note.c elf32_size.c \
	elf64_header.c elf64_phdrs.c elf64_shdrs.c elf64_rel.c \
	elf64_symload.c elf64_sym.c elf64_note.c elf64_size.c \
	checkoff.c
CPPFLAGS+=-I${.CURDIR}
CFLAGS+=-Wall
CLEANFILES+=elf32*.c elf64*.c
CLEANFILES+=elf32_dwarfnebula.c elf64_dwarfnebula.c
MAN=	elf_size.3
MLINKS=	elf_size.3 elf_fix_header.3 elf_size.3 elf_chk_header.3 \
	elf_size.3 elf_fix_note.3 elf_size.3 elf_save_phdrs.3 \
	elf_size.3 elf_load_shdrs.3 elf_size.3 elf_save_shdrs.3 \
	elf_size.3 elf_fix_shdrs.3 elf_size.3 elf_shstrload.3 \
	elf_size.3 elf_load_phdrs.3 elf_size.3 elf_fix_phdrs.3 \
	elf_size.3 elf_strload.3 elf_size.3 elf_symload.3 \
	elf_size.3 elf_fix_sym.3 elf_size.3 elf2nlist.3 \
	elf_size.3 elf_fix_rel.3 elf_size.3 elf_fix_rela.3

elf32_header.c: ${.CURDIR}/elf_header.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

elf64_header.c: ${.CURDIR}/elf_header.c
	echo '#define ELFSIZE 64' | cat - $> > ${.TARGET}

elf32_phdrs.c: ${.CURDIR}/elf_phdrs.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

elf64_phdrs.c: ${.CURDIR}/elf_phdrs.c
	echo '#define ELFSIZE 64' | cat - $> > ${.TARGET}

elf32_shdrs.c: ${.CURDIR}/elf_shdrs.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

elf64_shdrs.c: ${.CURDIR}/elf_shdrs.c
	echo '#define ELFSIZE 64' | cat - $> > ${.TARGET}

elf32_symload.c: ${.CURDIR}/elf_symload.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

elf64_symload.c: ${.CURDIR}/elf_symload.c
	echo '#define ELFSIZE 64' | cat - $> > ${.TARGET}

elf32_sym.c: ${.CURDIR}/elf_sym.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

elf64_sym.c: ${.CURDIR}/elf_sym.c
	echo '#define ELFSIZE 64' | cat - $> > ${.TARGET}

elf32_rel.c: ${.CURDIR}/elf_rel.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

elf64_rel.c: ${.CURDIR}/elf_rel.c
	echo '#define ELFSIZE 64' | cat - $> > ${.TARGET}

elf32_note.c: ${.CURDIR}/elf_note.c
	echo '#define ELFSIZE 32' | cat - $> > ${.TARGET}

elf64_note.c: ${.CURDIR}/elf_note.c
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
