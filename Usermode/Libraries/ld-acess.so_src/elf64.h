/*
 * Acess2 Dynamic Linker
 *
 * elf64.h
 */

#ifndef _ELF64_H_
#define _ELF64_H_

#define ELFCLASS64	2

#define EM_X86_64	62

typedef uint16_t	Elf64_Half;
typedef uint32_t	Elf64_Word;

typedef uint64_t	Elf64_Addr;
typedef uint64_t	Elf64_Off;
typedef uint64_t	Elf64_Xword;
typedef  int64_t	Elf64_Sxword;

typedef struct
{
	unsigned char	e_ident[16];
	Elf64_Half	e_type;
	Elf64_Half	e_machine;
	Elf64_Word	e_version;
	Elf64_Addr	e_entry;
	Elf64_Off	e_phoff;
	Elf64_Off	e_shoff;
	Elf64_Word	e_flags;
	Elf64_Half	e_ehsize;
	Elf64_Half	e_phentsize;
	Elf64_Half	e_phnum;
	Elf64_Half	e_shentsize;
	Elf64_Half	e_shnum;
	Elf64_Half	e_shstrndx;
} __attribute__((packed)) Elf64_Ehdr;

typedef struct
{
	Elf64_Word	sh_name;
	Elf64_Word	sh_type;
	Elf64_Xword	sh_flags;
	Elf64_Addr	sh_addr;
	Elf64_Off	sh_offset;
	Elf64_Xword	sh_size;
	Elf64_Word	sh_link;
	Elf64_Word	sh_info;
	Elf64_Xword	sh_addralign;
	Elf64_Xword	sh_entsize;
} Elf64_Shdr;

typedef struct
{
	Elf64_Word	p_type;
	Elf64_Word	p_flags;
	Elf64_Off	p_offset;
	Elf64_Addr	p_vaddr;
	Elf64_Addr	p_paddr;
	Elf64_Xword	p_filesz;
	Elf64_Xword	p_memsz;
	Elf64_Xword	p_align;
} Elf64_Phdr;

typedef struct
{
	Elf64_Sxword	d_tag;
	union {
		Elf64_Xword	d_val;
		Elf64_Addr	d_ptr;
	} d_un;
} Elf64_Dyn;

typedef struct
{
	Elf64_Word	st_name;
	uint8_t 	st_info;
	uint8_t 	st_other;
	Elf64_Half	st_shndx;
	Elf64_Addr	st_value;
	Elf64_Xword	st_size;
} Elf64_Sym;

typedef struct
{
	Elf64_Addr	r_offset;
	Elf64_Xword	r_info;
} Elf64_Rel;

typedef struct
{
	Elf64_Addr	r_offset;
	Elf64_Xword	r_info;
	Elf64_Sxword	r_addend;
} Elf64_Rela;

#define ELF64_R_SYM(info)	((info) >> 32)
#define ELF64_R_TYPE(info)	((info) & 0xFFFFFFFF)

enum eElf64_RelocTypes_x86_64
{
	R_X86_64_NONE,
	R_X86_64_64,	// 64, S + A
	R_X86_64_PC32,	// 32, S + A - P
	R_X86_64_GOT32,	// 32, G + A
	R_X86_64_PLT32,	// 32, L + A - P
	R_X86_64_COPY,
	R_X86_64_GLOB_DAT,	// 64, S
	R_X86_64_JUMP_SLOT,	// 64, S
	R_X86_64_RELATIVE,	// 64, B + A
	// TODO: Rest
};

#endif

