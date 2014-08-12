
LIB=	elf
SRCS=	checkoff.c dwarf_bytes.c dwarf_info.c dwarf_line.c dwarf_names.c
SRCS2=	elf_header.c elf_phdrs.c elf_shdrs.c elf_rel.c \
	elf_symload.c elf_sym.c elf_note.c elf_size.c \
	elf_dwarfnebula.c
CPPFLAGS+=-I${.CURDIR}
CFLAGS+=-Wall
MAN=	elf_size.3
MLINKS=	elf_size.3 elf_fix_header.3 elf_size.3 elf_chk_header.3 \
	elf_size.3 elf_fix_note.3 elf_size.3 elf_shstrload.3 \
	elf_size.3 elf_load_shdrs.3 elf_size.3 elf_save_shdrs.3 \
	elf_size.3 elf_fix_shdrs.3 elf_size.3 elf_scan_shdrs.3 \
	elf_size.3 elf_load_phdrs.3 elf_size.3 elf_fix_phdrs.3 \
	elf_size.3 elf_save_phdrs.3 elf_size.3 elf_scan_phdrs.3 \
	elf_size.3 elf_strload.3 elf_size.3 elf_symload.3 \
	elf_size.3 elf_fix_sym.3 elf_size.3 elf2nlist.3 \
	elf_size.3 elf_fix_rel.3 elf_size.3 elf_fix_rela.3 \
	elf_size.3 elf_dwarfnebula.3

.for F in ${SRCS2}
SRCS+=${F:S/elf_/elf32_/}
CLEANFILES+=${F:S/elf_/elf32_/}
${F:S/elf_/elf32_/}: ${.CURDIR}/$F
	printf '#define ELFSIZE 32\n#line 1 \"%s\"\n' $> | \
	    cat - $> > ${.TARGET}

SRCS+=${F:S/elf_/elf64_/}
CLEANFILES+=${F:S/elf_/elf64_/}
${F:S/elf_/elf64_/}: ${.CURDIR}/$F
	printf '#define ELFSIZE 64\n#line 1 \"%s\"\n' $> | \
	    cat - $> > ${.TARGET}
.endfor

includes:
	@cmp -s ${DESTDIR}/usr/include/elfuncs.h ${.CURDIR}/elfuncs.h || \
	${INSTALL} ${INSTALL_COPY} -m 444 -o $(BINOWN) -g $(BINGRP) \
	    ${.CURDIR}/elfuncs.h ${DESTDIR}/usr/include/elfuncs.h

.include <bsd.lib.mk>
