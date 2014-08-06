/*
 * Copyright (c) 2014 Michael Shalayeff
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef __FreeBSD__
#define swap16	bswap16
#define swap32	bswap32
#define swap64	bswap64
#endif

#if ELFSIZE == 32
#define	swap_addr	swap32
#define	swap_off	swap32
#define	swap_sword	swap32
#define	swap_word	swap32
#define	swap_sxword	swap32
#define	swap_xword	swap32
#define	swap_half	swap16
#define	swap_quarter	swap16
#define	elf_shstrload	elf32_shstrload
#define	elf_dwarfnebula	elf32_dwarfnebula
#define	elf_lines_cmp	elf32_lines_cmp
#define	elf_fix_header	elf32_fix_header
#define	elf_chk_header	elf32_chk_header
#define	elf_load_phdrs	elf32_load_phdrs
#define	elf_scan_phdrs	elf32_scan_phdrs
#define	elf_scan_phdr	elf32_scan_phdr
#define	elf_save_phdrs	elf32_save_phdrs
#define	elf_fix_phdrs	elf32_fix_phdrs
#define	elf_fix_phdr	elf32_fix_phdr
#define	elf_load_shdrs	elf32_load_shdrs
#define	elf_save_shdrs	elf32_save_shdrs
#define	elf_scan_shdrs	elf32_scan_shdrs
#define	elf_scan_shdr	elf32_scan_shdr
#define	elf_fix_shdrs	elf32_fix_shdrs
#define	elf_fix_shdr	elf32_fix_shdr
#define	elf_shstrload	elf32_shstrload
#define	elf_fix_sym	elf32_fix_sym
#define	elf2nlist	elf32_2nlist
#define	elf_shn2type	elf32_shn2type
#define	elf_load_shdrs	elf32_load_shdrs
#define	elf_sld		elf32_sld
#define	elf_shstrload	elf32_shstrload
#define	elf_strload	elf32_strload
#define	elf_symloadx	elf32_symloadx
#define	elf_symload	elf32_symload
#define	elf_stab_cmp	elf32_stab_cmp
#define	elf_size	elf32_size
#define	elf_size_add	elf32_size_add
#define	elf_fix_note	elf32_fix_note
#define	elf_fix_rel	elf32_fix_rel
#define	elf_fix_rela	elf32_fix_rela
#elif ELFSIZE == 64
#define	swap_addr	swap64
#define	swap_off	swap64
#ifdef __alpha__
#define	swap_sword	swap64
#define	swap_word	swap64
#else
#define	swap_sword	swap32
#define	swap_word	swap32
#endif
#define	swap_sxword	swap64
#define	swap_xword	swap64
#define	swap_half	swap32
#define	swap_quarter	swap16
#define	elf_shstrload	elf64_shstrload
#define	elf_dwarfnebula	elf64_dwarfnebula
#define	elf_lines_cmp	elf64_lines_cmp
#define	elf_fix_header	elf64_fix_header
#define	elf_chk_header	elf64_chk_header
#define	elf_load_phdrs	elf64_load_phdrs
#define	elf_scan_phdrs	elf64_scan_phdrs
#define	elf_scan_phdr	elf64_scan_phdr
#define	elf_save_phdrs	elf64_save_phdrs
#define	elf_fix_phdrs	elf64_fix_phdrs
#define	elf_fix_phdr	elf64_fix_phdr
#define	elf_load_shdrs	elf64_load_shdrs
#define	elf_save_shdrs	elf64_save_shdrs
#define	elf_scan_shdrs	elf64_scan_shdrs
#define	elf_scan_shdr	elf64_scan_shdr
#define	elf_fix_shdrs	elf64_fix_shdrs
#define	elf_fix_shdr	elf64_fix_shdr
#define	elf_shstrload	elf64_shstrload
#define	elf_fix_sym	elf64_fix_sym
#define	elf2nlist	elf64_2nlist
#define	elf_shn2type	elf64_shn2type
#define	elf_load_shdrs	elf64_load_shdrs
#define	elf_sld		elf64_sld
#define	elf_shstrload	elf64_shstrload
#define	elf_strload	elf64_strload
#define	elf_symloadx	elf64_symloadx
#define	elf_symload	elf64_symload
#define	elf_stab_cmp	elf64_stab_cmp
#define	elf_size	elf64_size
#define	elf_size_add	elf64_size_add
#define	elf_fix_note	elf64_fix_note
#define	elf_fix_rel	elf64_fix_rel
#define	elf_fix_rela	elf64_fix_rela
#else
#error "Unsupported ELF class"
#endif

uint16_t dwarf_fix16(struct dwarf_nebula *, uint16_t);
uint32_t dwarf_fix32(struct dwarf_nebula *, uint32_t);
uint64_t dwarf_fix64(struct dwarf_nebula *, uint64_t);
