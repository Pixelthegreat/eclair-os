/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Common data structures, types and macros for the ELF file format
 */
#ifndef EC_ELF_H
#define EC_ELF_H

#include <stdint.h>

/* common types */
typedef uint16_t elf32_half_t;
typedef uint32_t elf32_off_t;
typedef uint32_t elf32_addr_t;
typedef uint32_t elf32_word_t;
typedef int32_t elf32_sword_t;

/* main header */
#define ELF_NIDENT 16

#define ELF_IDENT_MAG 0
#define ELF_IDENT_CLASS 4
#define ELF_IDENT_DATA 5

#define ELF_MAG_BYTES {'\x7f','E','L','F'}

#define ELF_CLASS_32 1
#define ELF_DATA_2LSB 1

#define ELF_TYPE_NONE 0
#define ELF_TYPE_REL 1
#define ELF_TYPE_EXEC 2

#define ELF_MACHINE_386 3

#define ELF_VERSION_CURRENT 1

typedef struct elf32_header {
	uint8_t ident[ELF_NIDENT]; /* identification bytes */
	elf32_half_t type; /* type of elf file */
	elf32_half_t machine; /* target machine */
	elf32_word_t version; /* version of format */
	elf32_addr_t entry; /* entry point of code execution */
	elf32_off_t phoff; /* offset of program headers */
	elf32_off_t shoff; /* offset of section headers */
	elf32_word_t flags; /* file flags */
	elf32_half_t ehsize; /* size of this header */
	elf32_half_t phentsize; /* size of program header */
	elf32_half_t phnum; /* number of program headers */
	elf32_half_t shentsize; /* size of section header */
	elf32_half_t shnum; /* number of section headers */
	elf32_half_t shstrndx; /* section header string table index */
} __attribute__((packed)) elf32_header_t;

/* section header */
#define ELF_SH_TYPE_NULL 0
#define ELF_SH_TYPE_PROGBITS 1
#define ELF_SH_TYPE_SYMTAB 2
#define ELF_SH_TYPE_STRTAB 3
#define ELF_SH_TYPE_RELA 4
#define ELF_SH_TYPE_NOBITS 8
#define ELF_SH_TYPE_REL 9

#define ELF_SH_FLAG_WRITE 0x01
#define ELF_SH_FLAG_ALLOC 0x02

typedef struct elf32_section_header {
	elf32_word_t name; /* name of section */
	elf32_word_t type; /* type of section */
	elf32_word_t flags; /* section flags */
	elf32_addr_t addr; /* virtual address of section */
	elf32_off_t offset; /* file offset of section */
	elf32_word_t size; /* size of section */
	elf32_word_t link; /* ??? */
	elf32_word_t info; /* ??? */
	elf32_word_t addralign; /* alignment of address */
	elf32_word_t entsize; /* ??? */
} __attribute__((packed)) elf32_section_header_t;

/* program header */
#define ELF_PH_TYPE_LOAD 1

typedef struct elf32_program_header {
	elf32_word_t type; /* type of program header */
	elf32_off_t offset; /* file offset of data */
	elf32_addr_t vaddr; /* virtual address */
	elf32_addr_t paddr; /* physical address */
	elf32_word_t filesz; /* size of data in file */
	elf32_word_t memsz; /* size of data in memory */
	elf32_word_t flags; /* program header flags */
	elf32_word_t align; /* alignment */
} __attribute__((packed)) elf32_program_header_t;

#endif /* EC_ELF_H */
