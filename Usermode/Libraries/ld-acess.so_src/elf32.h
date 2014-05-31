/**
 Acess v1
 \file elf32.h
 \brief ELF Exeutable Loader
*/
#ifndef _ELF32_H
#define _ELF32_H

#include <stdint.h>

typedef uint16_t	Elf32_Half;
typedef uint32_t	Elf32_Addr;
typedef uint32_t	Elf32_Off;
typedef uint32_t	Elf32_Word;
typedef int32_t 	Elf32_Sword;

#define ELFCLASS32	1

#define EM_NONE	0
#define EM_386	3
#define EM_ARM	40

/**
 \struct elf_header_s
 \brief ELF File Header
*/
struct sElf32_Ehdr {
	uint8_t	e_ident[16];	//!< Identifier Bytes
	Elf32_Half	filetype;	//!< File Type
	Elf32_Half	machine;	//!< Machine / Arch
	Elf32_Word	version;	//!< Version (File?)
	Elf32_Addr	entrypoint;	//!< Entry Point
	Elf32_Off	phoff;	//!< Program Header Offset
	Elf32_Word	shoff;	//!< Section Header Offset
	Elf32_Word	flags;	//!< Flags
	Elf32_Half	headersize;	//!< Header Size
	Elf32_Half	phentsize;	//!< Program Header Entry Size
	Elf32_Half	phentcount;	//!< Program Header Entry Count
	Elf32_Half	shentsize;	//!< Section Header Entry Size
	Elf32_Half	shentcount;	//!< Section Header Entry Count
	Elf32_Half	shstrindex;	//!< Section Header String Table Index
};

/**
 \name Executable Types
 \{
*/
#define	ET_NONE		0	//!< NULL Type
#define	ET_REL		1	//!< Relocatable (Object)
#define ET_EXEC		2	//!< Executable
#define ET_DYN		3	//!< Dynamic Library
#define ET_CORE		4	//!< Core?
#define ET_LOPROC	0xFF00	//!< Low Impl Defined
#define ET_HIPROC	0xFFFF	//!< High Impl Defined
//! \}

/**
 \name Section IDs
 \{
*/
#define SHN_UNDEF		0	//!< Undefined Section
#define SHN_LORESERVE	0xFF00	//!< Low Reserved
#define SHN_LOPROC		0xFF00	//!< Low Impl Defined
#define SHN_HIPROC		0xFF1F	//!< High Impl Defined
#define SHN_ABS			0xFFF1	//!< Absolute Address (Base: 0, Size: -1)
#define SHN_COMMON		0xFFF2	//!< Common
#define SHN_HIRESERVE	0xFFFF	//!< High Reserved
//! \}

/**
 \enum eElfSectionTypes
 \brief ELF Section Types
*/
enum eElfSectionTypes {
	SHT_NULL,	//0
	SHT_PROGBITS,	//1
	SHT_SYMTAB,	//2
	SHT_STRTAB,	//3
	SHT_RELA,	//4
	SHT_HASH,	//5
	SHT_DYNAMIC,	//6
	SHT_NOTE,	//7
	SHT_NOBITS,	//8
	SHT_REL,	//9
	SHT_SHLIB,	//A
	SHT_DYNSYM,	//B
	SHT_LAST,	//C
	SHT_LOPROC = 0x70000000,
	SHT_HIPROC = 0x7fffffff,
	SHT_LOUSER = 0x80000000,
	SHT_HIUSER = 0xffffffff
};

#if 0
#define SHF_WRITE	0x1
#define SHF_ALLOC	0x2
#define SHF_EXECINSTR	0x4
#define SHF_MASKPROC	0xf0000000

struct sElf32_Shent {
	Uint32	name;
	Uint32	type;
	Uint32	flags;
	Uint32	address;
	Uint32	offset;
	Uint32	size;
	Uint32	link;
	Uint32	info;
	Uint32	addralign;
	Uint32	entsize;
};	//sizeof = 40
#endif

struct elf_sym_s {
	Elf32_Word	st_name;
	Elf32_Addr	st_value;	//Address
	Elf32_Word	st_size;
	uint8_t 	st_info;
	uint8_t 	st_other;
	Elf32_Half	st_shndx;
};
#define	STN_UNDEF	0	// Undefined Symbol

#define ELF32_ST_BIND(i)	((i)>>4)
#define ELF32_ST_TYPE(i)	((i)&0xF)

enum {
	STB_LOCAL,
	STB_GLOBAL,
	STB_WEAK,
};

enum {
	PT_NULL,	//0
	PT_LOAD,	//1
	PT_DYNAMIC,	//2
	PT_INTERP,	//3
	PT_NOTE,	//4
	PT_SHLIB,	//5
	PT_PHDR,	//6
	PT_LOPROC = 0x70000000,
	PT_HIPROC = 0x7fffffff
};

#define PF_X	1
#define PF_W	2
#define PF_R	4

struct sElf32_Phdr {
	Elf32_Word	Type;
	Elf32_Off	Offset;
	Elf32_Addr	VAddr;
	Elf32_Addr	PAddr;
	Elf32_Word	FileSize;
	Elf32_Word	MemSize;
	Elf32_Word	Flags;
	Elf32_Word	Align;
};

struct elf32_rel_s {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
};

struct elf32_rela_s {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
	Elf32_Sword	r_addend;
};

enum {
	R_386_NONE=0,	// none
	R_386_32,	// S+A
	R_386_PC32,	// S+A-P
	R_386_GOT32,	// G+A-P
	R_386_PLT32,	// L+A-P
	R_386_COPY,	// none
	R_386_GLOB_DAT,	// S
	R_386_JMP_SLOT,	// S
	R_386_RELATIVE,	// B+A
	R_386_GOTOFF,	// S+A-GOT
	R_386_GOTPC,	// GOT+A-P
	R_386_LAST	// none
};

// NOTES:
//  'T' means the thumb bit
//  'B(S)' Origin of a symbol
enum {
	R_ARM_NONE,	//  0 No action
	R_ARM_PC24,	//  1 ((S + A) | T) - P
	R_ARM_ABS32,	//  2 (S + A) | T
	R_ARM_REL32,	//  3 ((S + A) | T) - P
	R_ARM_LDR_PC_G0,	//  4 S + A - P
	R_ARM_ABS16,	//  5 S + A
	R_ARM_ABS12,	//  6 S + A
	R_ARM_THM_ABS5,	//  7 S + A
	R_ARM_ABS8,	//  8 S + A
	R_ARM_SBREL32,	//  9 ((S + A) | T) - B(S)
	R_ARM_THM_CALL,	// 10 ((S + A) | T) - P
	R_ARM_THM_PC8,	// 11 S + A - Pa,
	R_ARM_BREL_ADJ,	// 12 ΔB(S) + A
	R_ARM_TLS_DESC,	// 13 --
	R_ARM_THM_SWI8,	// 14 (Reserved)
	R_ARM_XPC25,	// 15 (Reserved)
	R_ARM_THM_XPC22,	// 16 (Reserved)
	R_ARM_TLS_DTPMOD32,	// 17 Module[S]
	R_ARM_TLS_DTPOFF32,	// 18 S + A - TLS
	R_ARM_TLS_TPOFF32,	// 19 S + A - tp
	R_ARM_COPY,	// 20 Misc
	R_ARM_GLOB_DAT,	// 21 (S + A) | T
	R_ARM_JUMP_SLOT,	// 22 (S + A) | T
	R_ARM_RELATIVE,	// 23 B(S) + A (extra?)
	// ... More defined (IHI0044)
};

#define	ELF32_R_SYM(i)	((i)>>8)	// Takes an info value and returns a symbol index
#define	ELF32_R_TYPE(i)	((i)&0xFF)	// Takes an info value and returns a type
#define	ELF32_R_INFO(s,t)	(((s)<<8)+((t)&0xFF))	// Takes a type and symbol index and returns an info value

struct elf32_dyn_s {
	Elf32_Half	d_tag;
	Elf32_Word	d_val;	//Also d_ptr
};

enum {
	DT_NULL,	//!< Marks End of list
	DT_NEEDED,	//!< Offset in strtab to needed library
	DT_PLTRELSZ,	//!< Size in bytes of PLT
	DT_PLTGOT,	//!< Address of PLT/GOT
	DT_HASH,	//!< Address of symbol hash table
	DT_STRTAB,	//!< String Table address
	DT_SYMTAB,	//!< Symbol Table address
	DT_RELA,	//!< Relocation table address
	DT_RELASZ,	//!< Size of relocation table
	DT_RELAENT,	//!< Size of entry in relocation table
	DT_STRSZ,	//!< Size of string table
	DT_SYMENT,	//!< Size of symbol table entry
	DT_INIT,	//!< Address of initialisation function
	DT_FINI,	//!< Address of termination function
	DT_SONAME,	//!< String table offset of so name
	DT_RPATH,	//!< String table offset of library path
	DT_SYMBOLIC,//!< Reverse order of symbol searching for library, search libs first then executable
	DT_REL,		//!< Relocation Entries (Elf32_Rel instead of Elf32_Rela)
	DT_RELSZ,	//!< Size of above table (bytes)
	DT_RELENT,	//!< Size of entry in above table
	DT_PLTREL,	//!< Relocation entry of PLT
	DT_DEBUG,	//!< Debugging Entry - Unknown contents
	DT_TEXTREL,	//!< Indicates that modifcations to a non-writeable segment may occur
	DT_JMPREL,	//!< Address of PLT only relocation entries
	DT_LOPROC = 0x70000000,	//!< Low Definable
	DT_HIPROC = 0x7FFFFFFF	//!< High Definable
};

typedef struct sElf32_Ehdr	Elf32_Ehdr;
typedef struct sElf32_Phdr	Elf32_Phdr;
typedef struct sElf32_Shent	Elf32_Shent;
typedef struct elf_sym_s	elf_symtab;
typedef struct elf_sym_s	Elf32_Sym;
typedef struct elf32_rel_s	Elf32_Rel;
typedef struct elf32_rela_s	Elf32_Rela;
typedef struct elf32_dyn_s	Elf32_Dyn;

#endif	// defined(_EXE_ELF_H)
