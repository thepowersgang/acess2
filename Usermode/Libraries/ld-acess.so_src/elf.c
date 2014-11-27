/*
 * Acess2 Dynamic Linker
 * - By John Hodge (thePowersGang)
 *
 * elf.c
 * - ELF32/ELF64 relocation
 *
 * TODO: Have GetSymbol() return a symbol "strength" on success. Allows STB_WEAK to be overriden by STB_GLOBAL
 */
#ifndef KERNEL_VERSION
# define DEBUG	0
#endif

#ifndef PAGE_SIZE
# define PAGE_SIZE	4096
#endif

#include "common.h"
#include <stdint.h>
#include <stdbool.h>
#ifndef assert
# include <assert.h>
#endif
#include "elf32.h"
#include "elf64.h"

#if DEBUG
# define DEBUG_OUT(...)	SysDebug(__VA_ARGS__)
#else
# define DEBUG_OUT(...)	do{}while(0)	//((void)(__VA_ARGS__))
#endif

#define WARNING(f,...)	SysDebug("WARN: "f ,## __VA_ARGS__)	// Malformed file
#define NOTICE(f,...)	SysDebug("NOTICE: "f ,## __VA_ARGS__)	// Missing relocation
//#define TRACE(f,...)	DEBUG_OUT("TRACE:%s:%i "f, __func__, __LINE__ ,## __VA_ARGS__)	// Debugging trace
#define TRACE(f,...)	DEBUG_OUT("TRACE:%s "f, __func__,## __VA_ARGS__)	// Debugging trace

#ifndef DISABLE_ELF64
# define SUPPORT_ELF64
#endif

// === CONSTANTS ===
#if DEBUG
//static const char	*csaDT_NAMES[] = {"DT_NULL", "DT_NEEDED", "DT_PLTRELSZ", "DT_PLTGOT", "DT_HASH", "DT_STRTAB", "DT_SYMTAB", "DT_RELA", "DT_RELASZ", "DT_RELAENT", "DT_STRSZ", "DT_SYMENT", "DT_INIT", "DT_FINI", "DT_SONAME", "DT_RPATH", "DT_SYMBOLIC", "DT_REL", "DT_RELSZ", "DT_RELENT", "DT_PLTREL", "DT_DEBUG", "DT_TEXTREL", "DT_JMPREL"};
//static const char	*csaR_NAMES[] = {"R_386_NONE", "R_386_32", "R_386_PC32", "R_386_GOT32", "R_386_PLT32", "R_386_COPY", "R_386_GLOB_DAT", "R_386_JMP_SLOT", "R_386_RELATIVE", "R_386_GOTOFF", "R_386_GOTPC", "R_386_LAST"};
#endif

#ifdef SUPPORT_ELF64
#endif

// === PROTOTYPES ===
void	*ElfRelocate(void *Base, char **envp, const char *Filename);
 int	ElfGetSymbol(void *Base, const char *Name, void **Ret, size_t *Size);
void	*Elf32_Relocate(void *Base, char **envp, const char *Filename);
 int	Elf32_GetSymbol(void *Base, const char *Name, void **Ret, size_t *Size);
#ifdef SUPPORT_ELF64
void	*Elf64_Relocate(void *Base, char **envp, const char *Filename);
 int	Elf64_GetSymbol(void *Base, const char *Name, void **Ret, size_t *Size);
#endif
uint32_t	ElfHashString(const char *name);

// === CODE ===
/**
 * \fn int ElfRelocate(void *Base, char **envp, const char *Filename)
 * \brief Relocates a loaded ELF Executable
 */
void *ElfRelocate(void *Base, char **envp, const char *Filename)
{
	Elf32_Ehdr	*hdr = Base;
	
	switch(hdr->e_ident[4])
	{
	case ELFCLASS32:
		return Elf32_Relocate(Base, envp, Filename);
#ifdef SUPPORT_ELF64
	case ELFCLASS64:
		return Elf64_Relocate(Base, envp, Filename);
#endif
	default:
		SysDebug("ld-acess - ElfRelocate: Unknown file class %i", hdr->e_ident[4]);
		return NULL;
	}
}

/**
 * \fn int ElfGetSymbol(Uint Base, const char *name, void **ret)
 */
int ElfGetSymbol(void *Base, const char *Name, void **ret, size_t *Size)
{
	Elf32_Ehdr	*hdr = Base;

	switch(hdr->e_ident[4])
	{
	case ELFCLASS32:
		return Elf32_GetSymbol(Base, Name, ret, Size);
#ifdef SUPPORT_ELF64
	case ELFCLASS64:
		return Elf64_GetSymbol(Base, Name, ret, Size);
#endif
	default:
		SysDebug("ld-acess - ElfRelocate: Unknown file class %i", hdr->e_ident[4]);
		return 0;
	}
}

// --------------------------------------------------------------------
// Elf32 support
// --------------------------------------------------------------------
#define ELFTYPE	Elf32
#include "elf_impl.c"
#undef ELFTYPE

Elf32_RelocFcn	elf_doRelocate_386;
Elf32_RelocFcn	elf_doRelocate_arm;

int elf_doRelocate_386(const Elf32_RelocInfo *Info, Elf32_Word r_info, Elf32_Word* ptr, Elf32_Addr addend, bool bRela)
{
	const Elf32_Sym	*sym = &Info->symtab[ ELF32_R_SYM(r_info) ];
	void	*symval = (void*)(intptr_t)sym->st_value;
	size_t	size = sym->st_size;
	TRACE("%i '%s'", ELF32_R_TYPE(r_info), Info->strtab + sym->st_name);
	switch( ELF32_R_TYPE(r_info) )
	{
	// Standard 32 Bit Relocation (S+A)
	case R_386_32:
		TRACE("R_386_32 *0x%x = %p + 0x%x", ptr, symval, addend);
		*ptr = (intptr_t)symval + addend;
		break;
		
	// 32 Bit Relocation wrt. Offset (S+A-P)
	case R_386_PC32:
		TRACE("R_386_PC32 *0x%x = 0x%x + 0x%p - 0x%x", ptr, *ptr, symval, (intptr_t)ptr );
		*ptr = (intptr_t)symval + addend - (intptr_t)ptr;
		//*ptr = val + addend - ((Uint)ptr - iBaseDiff);
		break;

	// Absolute Value of a symbol (S)
	case R_386_GLOB_DAT:
		TRACE("R_386_GLOB_DAT *0x%x = %p", ptr, symval);	if(0)
	case R_386_JMP_SLOT:
		TRACE("R_386_JMP_SLOT *0x%x = %p", ptr, symval);
		*ptr = (intptr_t)symval;
		break;

	// Base Address (B+A)
	case R_386_RELATIVE:
		TRACE("R_386_RELATIVE *0x%x = 0x%x + 0x%x", ptr, Info->iBaseDiff, addend);
		*ptr = Info->iBaseDiff + addend;
		break;

	case R_386_COPY: {
		void *old_symval = symval;
		GetSymbol(Info->strtab + sym->st_name, &symval, &size, Info->Base);
		if( symval == old_symval )
		{
			if( ELF32_ST_BIND(sym->st_info) != STB_WEAK )
			{
				WARNING("sym={val:%p,size:0x%x,info:0x%x,other:0x%x,shndx:%i}",
					sym->st_value, sym->st_size, sym->st_info, sym->st_other, sym->st_shndx);
				WARNING("Can't find required external symbol '%s' for R_386_COPY", Info->strtab + sym->st_name);
				return 1;
			}
			// Don't bother doing the memcpy
			TRACE("R_386_COPY (%p, %p, %i)", ptr, symval, size);
		}
		else
		{
			TRACE("R_386_COPY (%p, %p, %i)", ptr, symval, size);
			memcpy(ptr, symval, size);
		}
		break; }

	default:
		WARNING("Unknown relocation %i", ELF32_ST_TYPE(r_info));
		return 2;
	}
	return 0;
}

int elf_doRelocate_arm(const Elf32_RelocInfo *Info, Elf32_Word r_info, Elf32_Word* ptr, Elf32_Addr addend, bool bRela)
{
	const Elf32_Sym	*sym = &Info->symtab[ ELF32_R_SYM(r_info) ];
	void	*symval = (void*)(intptr_t)sym->st_value;
	size_t	size = sym->st_size;
	TRACE("%i '%s'", ELF32_R_TYPE(r_info), Info->strtab + sym->st_name);
	uintptr_t	val = (uintptr_t)symval;
	switch( ELF32_R_TYPE(r_info) )
	{
	// (S + A) | T
	case R_ARM_ABS32:
		TRACE("R_ARM_ABS32 %p (%p + %x)", ptr, symval, addend);
		*ptr = val + addend;
		break;
	case R_ARM_GLOB_DAT:
		TRACE("R_ARM_GLOB_DAT %p (%p + %x)", ptr, symval, addend);
		*ptr = val + addend;
		break;
	case R_ARM_JUMP_SLOT:
		if(!bRela)	addend = 0;
		TRACE("R_ARM_JUMP_SLOT %p (%p + %x)", ptr, symval, addend);
		*ptr = val + addend;
		break;
	// Copy
	case R_ARM_COPY:
		TRACE("R_ARM_COPY (%p, %p, %i)", ptr, symval, size);
		memcpy(ptr, symval, size);
		break;
	// Delta between link and runtime locations + A
	case R_ARM_RELATIVE:
		TRACE("R_ARM_RELATIVE %p (0x%x + 0x%x)", ptr, Info->iBaseDiff, addend);
		if(ELF32_R_SYM(r_info) != 0) {
			// TODO: Get delta for a symbol
			WARNING("TODO - Implment R_ARM_RELATIVE for symbols");
			return 2;
		}
		else {
			*ptr = Info->iBaseDiff + addend;
		}
		break;
	default:
		WARNING("Unknown Relocation, %i", ELF32_R_TYPE(r_info));
		return 2;
	}
	return 0;
}

Elf32_RelocFcn*	Elf32_GetRelocFcn(unsigned Machine)
{
	switch(Machine)
	{
	case EM_386:	return elf_doRelocate_386;
	case EM_ARM:	return elf_doRelocate_arm;
	default:	return NULL;
	}
}


// --------------------------------------------------------------------
// Elf64 support
// --------------------------------------------------------------------
#ifdef SUPPORT_ELF64

#define ELFTYPE	Elf64
#include "elf_impl.c"
#undef ELFTYPE

Elf64_RelocFcn	elf_doRelocate_x86_64;

int elf_doRelocate_x86_64(const Elf64_RelocInfo *Info, Elf64_Xword r_info, Elf64_Xword* ptr, Elf64_Addr addend, bool bRela)
{
	const Elf64_Sym	*sym = &Info->symtab[ ELF64_R_SYM(r_info) ];
	void	*symval = (void*)(intptr_t)sym->st_value;
	size_t	size = sym->st_size;
	TRACE("%i '%s'", ELF64_R_TYPE(r_info), Info->strtab + sym->st_name);
	switch( ELF64_R_TYPE(r_info) )
	{
	case R_X86_64_NONE:
		break;
	case R_X86_64_64:
		TRACE("R_X86_64_64 *0x%x = %p + 0x%x", ptr, symval, addend);
		*ptr = (intptr_t)symval + addend;
		break;
	// Absolute Value of a symbol (S)
	case R_X86_64_GLOB_DAT:
		TRACE("R_X86_64_GLOB_DAT *0x%x = %p", ptr, symval);	if(0)
	case R_X86_64_JUMP_SLOT:
		TRACE("R_X86_64_JUMP_SLOT *0x%x = %p", ptr, symval);
		*ptr = (intptr_t)symval;
		break;

	// Base Address (B+A)
	case R_X86_64_RELATIVE:
		TRACE("R_X86_64_RELATIVE *0x%x = 0x%x + 0x%x", ptr, Info->iBaseDiff, addend);
		*ptr = Info->iBaseDiff + addend;
		break;

	case R_X86_64_COPY: {
		void *old_symval = symval;
		GetSymbol(Info->strtab + sym->st_name, &symval, &size, Info->Base);
		if( symval == old_symval )
		{
			if( ELF64_ST_BIND(sym->st_info) != STB_WEAK )
			{
				WARNING("sym={val:%p,size:0x%x,info:0x%x,other:0x%x,shndx:%i}",
					sym->st_value, sym->st_size, sym->st_info, sym->st_other, sym->st_shndx);
				WARNING("Can't find required external symbol '%s' for R_X86_64_COPY", Info->strtab + sym->st_name);
				return 1;
			}
			// Don't bother doing the memcpy
			TRACE("R_X86_64_COPY (%p, %p, %i)", ptr, symval, size);
		}
		else
		{
			TRACE("R_X86_64_COPY (%p, %p, %i)", ptr, symval, size);
			memcpy(ptr, symval, size);
		}
		break; }
	default:
		WARNING("Unknown Relocation, %i", ELF64_R_TYPE(r_info));
		return 2;
	}
	return 0;
}

Elf64_RelocFcn* Elf64_GetRelocFcn(unsigned Machine)
{
	switch(Machine)
	{
	case EM_X86_64:	return elf_doRelocate_x86_64;
	default:	return NULL;
	}
}

#endif	// SUPPORT_ELF64


#ifdef SUPPORT_ELF64
#if 0
typedef int (*t_elf64_doreloc)(void *Base, const char *strtab, Elf64_Sym *symtab, Elf64_Xword r_info, void *ptr, Elf64_Sxword addend);

int _Elf64DoReloc_X86_64(void *Base, const char *strtab, Elf64_Sym *symtab, Elf64_Xword r_info, void *ptr, Elf64_Sxword addend)
{
	 int	sym = ELF64_R_SYM(r_info);
	 int	type = ELF64_R_TYPE(r_info);
	const char	*symname = strtab + symtab[sym].st_name;
	void	*symval;
	//DEBUGS("_Elf64DoReloc: %s", symname);
	switch( type )
	{
	case R_X86_64_NONE:
		break;
	case R_X86_64_64:
		if( !GetSymbol(symname, &symval, NULL, NULL)  )	return 1;
		*(uint64_t*)ptr = (uintptr_t)symval + addend;
		break;
	case R_X86_64_COPY: {
		size_t	size;
		if( !GetSymbol(symname, &symval, &size, NULL)  )	return 1;
		memcpy(ptr, symval, size);
		} break;
	case R_X86_64_GLOB_DAT:
		if( !GetSymbol(symname, &symval, NULL, NULL)  )	return 1;
		*(uint64_t*)ptr = (uintptr_t)symval;
		break;
	case R_X86_64_JUMP_SLOT:
		if( !GetSymbol(symname, &symval, NULL, NULL)  )	return 1;
		*(uint64_t*)ptr = (uintptr_t)symval;
		break;
	case R_X86_64_RELATIVE:
		*(uint64_t*)ptr = (uintptr_t)Base + addend;
		break;
	default:
		SysDebug("ld-acess - _Elf64DoReloc: Unknown relocation type %i", type);
		return 2;
	}
	//DEBUGS("_Elf64DoReloc: - Good");
	return 0;
}

void *Elf64Relocate(void *Base, char **envp, const char *Filename)
{
	 int	i;
	Elf64_Ehdr	*hdr = Base;
	Elf64_Phdr	*phtab;
	Elf64_Dyn	*dyntab = NULL;
	Elf64_Addr	compiledBase = -1, baseDiff;
	Elf64_Sym	*symtab = NULL;
	char	*strtab = NULL;
	Elf64_Word	*hashtab = NULL;
	Elf64_Rel	*rel = NULL;
	 int	rel_count = 0;
	Elf64_Rela	*rela = NULL;
	 int	rela_count = 0;
	void	*pltrel = NULL;
	 int	plt_size = 0, plt_type = 0;

	TRACE("hdr = {");
	TRACE(" e_ident = '%.16s'", hdr->e_ident);
	TRACE(" e_type = 0x%x", hdr->e_type);
	TRACE(" e_machine = 0x%x", hdr->e_machine);
	TRACE(" e_version = 0x%x", hdr->e_version);
	TRACE(" e_entry = %p", hdr->e_entry);
	TRACE(" e_phoff = 0x%llx", hdr->e_phoff);
	TRACE(" e_shoff = 0x%llx", hdr->e_shoff);
	TRACE(" e_flags = 0x%x", hdr->e_flags);
	TRACE(" e_ehsize = 0x%x", hdr->e_ehsize);
	TRACE(" e_phentsize = 0x%x", hdr->e_phentsize);
	TRACE(" e_phnum = %i", hdr->e_phnum);

	// Scan for the dynamic table (and find the compiled base)
	phtab = (void*)((uintptr_t)Base + (uintptr_t)hdr->e_phoff);
	for( i = 0; i < hdr->e_phnum; i ++ )
	{
		if(phtab[i].p_type == PT_DYNAMIC)
			dyntab = (void *)(intptr_t)phtab[i].p_vaddr;
		if(phtab[i].p_type == PT_LOAD && compiledBase > phtab[i].p_vaddr)
			compiledBase = phtab[i].p_vaddr;
	}

	baseDiff = (uintptr_t)Base - compiledBase;

	TRACE("baseDiff = %p", baseDiff);

	if(dyntab == NULL) {
		SysDebug(" Elf64Relocate: No PT_DYNAMIC segment in image %p, returning", Base);
		return (void *)(uintptr_t)(hdr->e_entry + baseDiff);
	}

	dyntab = (void *)(uintptr_t)((uintptr_t)dyntab + baseDiff);

	// Parse the dynamic table (first pass)
	// - Search for String, Symbol and Hash tables
	for(i = 0; dyntab[i].d_tag != DT_NULL; i ++)
	{
		switch(dyntab[i].d_tag)
		{
		case DT_SYMTAB:
			dyntab[i].d_un.d_ptr += baseDiff;
			symtab = (void *)(uintptr_t)dyntab[i].d_un.d_ptr;
			break;
		case DT_STRTAB:
			dyntab[i].d_un.d_ptr += baseDiff;
			strtab = (void *)(uintptr_t)dyntab[i].d_un.d_ptr;
			break;
		case DT_HASH:
			dyntab[i].d_un.d_ptr += baseDiff;
			hashtab = (void *)(uintptr_t)dyntab[i].d_un.d_ptr;
			break;
		}
	}

	if( !symtab || !strtab || !hashtab ) {
		SysDebug("ld-acess - Elf64Relocate: Missing Symbol, string or hash table");
		return NULL;
	}

	// Ready for symbol use	
	AddLoaded( Filename, Base );

	// Second pass on dynamic table
	for(i = 0; dyntab[i].d_tag != DT_NULL; i ++)
	{
		TRACE("dyntab[%i].d_tag = %i", i, dyntab[i].d_tag);
		switch(dyntab[i].d_tag)
		{
		case DT_SONAME:	break;

		case DT_NEEDED: {
			char *libPath = strtab + dyntab[i].d_un.d_val;
			TRACE("Elf64Relocate: libPath = '%s'", libPath);
			if(LoadLibrary(libPath, NULL, envp) == 0) {
				SysDebug("ld-acess - Elf64Relocate: Unable to load '%s'", libPath);
				return NULL;
			}
			} break;
		
		// Relocation entries
		case DT_REL:
			dyntab[i].d_un.d_ptr += baseDiff;
			rel = (void *)(uintptr_t)dyntab[i].d_un.d_ptr;
			break;
		case DT_RELSZ:
			rel_count = dyntab[i].d_un.d_val / sizeof(Elf64_Rel);
			break;
		case DT_RELENT:
			if( dyntab[i].d_un.d_val != sizeof(Elf64_Rel) ) {
				SysDebug("ld-acess - Elf64Relocate: DT_RELENT(%i) != sizeof(Elf64_Rel)(%i)",
					dyntab[i].d_un.d_val, sizeof(Elf64_Rel));
				return NULL;
			}
			break;
		case DT_RELA:
			dyntab[i].d_un.d_ptr += baseDiff;
			rela = (void *)(uintptr_t)dyntab[i].d_un.d_ptr;
			break;
		case DT_RELASZ:
			rela_count = dyntab[i].d_un.d_val / sizeof(Elf64_Rela);
			break;
		case DT_RELAENT:
			if( dyntab[i].d_un.d_val != sizeof(Elf64_Rela) ) {
				SysDebug("ld-acess - Elf64Relocate: DT_RELAENT(%i) != sizeof(Elf64_Rela)(%i)",
					dyntab[i].d_un.d_val, sizeof(Elf64_Rela));
				return NULL;
			}
			break;
		case DT_JMPREL:
			dyntab[i].d_un.d_ptr += baseDiff;
			pltrel = (void *)(uintptr_t)dyntab[i].d_un.d_ptr;
			break;
		case DT_PLTREL:
			plt_type = dyntab[i].d_un.d_val;
			break;
		case DT_PLTRELSZ:
			plt_size = dyntab[i].d_un.d_val;
			break;
		}
	}

	// TODO: Relocate symbols
	
	// Relocation function
	t_elf64_doreloc fpElf64DoReloc = &_Elf64DoReloc_X86_64;
	#define _Elf64DoReloc(info, ptr, addend)	fpElf64DoReloc(Base, strtab, symtab, info, ptr, addend)

	int fail = 0;
	if( rel )
	{
		TRACE("rel_count = %i", rel_count);
		for( i = 0; i < rel_count; i ++ )
		{
			uint64_t *ptr = (void *)(uintptr_t)( rel[i].r_offset + baseDiff );
			fail |= _Elf64DoReloc( rel[i].r_info, ptr, *ptr);
		}
	}

	if( rela )
	{
		TRACE("rela_count = %i", rela_count);
		for( i = 0; i < rela_count; i ++ )
		{
			uint64_t *ptr = (void *)(uintptr_t)( rela[i].r_offset + baseDiff );
			fail |= _Elf64DoReloc( rela[i].r_info, ptr, rela[i].r_addend );
		}
	}

	if( pltrel && plt_type )
	{
		if( plt_type == DT_REL ) {
			Elf64_Rel	*plt = pltrel;
			 int	count = plt_size / sizeof(Elf64_Rel);
			TRACE("plt rel count = %i", count);
			for( i = 0; i < count; i ++ )
			{
				uint64_t *ptr = (void *)(uintptr_t)( plt[i].r_offset + baseDiff );
				fail |= _Elf64DoReloc( plt[i].r_info, ptr, *ptr);
			}
		}
		else {
			Elf64_Rela	*plt = pltrel;
			 int	count = plt_size / sizeof(Elf64_Rela);
			TRACE("plt rela count = %i", count);
			for( i = 0; i < count; i ++ )
			{
				uint64_t *ptr = (void *)(uintptr_t)( plt[i].r_offset + baseDiff );
				fail |= _Elf64DoReloc( plt[i].r_info, ptr, plt[i].r_addend);
			}
		}
	}

	if( fail ) {
		TRACE("Failure");
		return NULL;
	}

	{
	void *ret = (void *)(uintptr_t)(hdr->e_entry + baseDiff);
	TRACE("Relocations done, return %p", ret);
	return ret;
	}
}

int Elf64GetSymbol(void *Base, const char *Name, void **Ret, size_t *Size)
{
	Elf64_Ehdr	*hdr = Base;
	Elf64_Sym	*symtab;
	 int	nbuckets = 0;
//	 int	iSymCount = 0;
	 int	i;
	Elf64_Word	*pBuckets;
	Elf64_Word	*pChains;
	uint32_t	iNameHash;
	const char	*dynstrtab;
	uintptr_t	iBaseDiff = -1;

	dynstrtab = NULL;
	pBuckets = NULL;
	symtab = NULL;

	// Catch the current executable
	if( !pBuckets )
	{
		Elf64_Phdr	*phtab;
		Elf64_Dyn	*dynTab = NULL;
		 int	j;
		
		// Locate the tables
		phtab = (void*)( (intptr_t)Base + (uintptr_t)hdr->e_phoff );
		for( i = 0; i < hdr->e_phnum; i ++ )
		{
			if(phtab[i].p_type == PT_LOAD && iBaseDiff > phtab[i].p_vaddr)
				iBaseDiff = phtab[i].p_vaddr;
			if( phtab[i].p_type == PT_DYNAMIC ) {
				dynTab = (void*)(intptr_t)phtab[i].p_vaddr;
			}
		}
		if( !dynTab ) {
			SysDebug("ERROR - Unable to find DYNAMIC segment in %p", Base);
			return 0;
		}
		iBaseDiff = (intptr_t)Base - iBaseDiff;	// Make iBaseDiff actually the diff
		dynTab = (void*)( (intptr_t)dynTab + iBaseDiff );
		
		for( j = 0; dynTab[j].d_tag != DT_NULL; j++)
		{
			switch(dynTab[j].d_tag)
			{
			// --- Symbol Table ---
			case DT_SYMTAB:
				symtab = (void*)(intptr_t) dynTab[j].d_un.d_val;	// Rebased in Relocate
				break;
			case DT_STRTAB:
				dynstrtab = (void*)(intptr_t) dynTab[j].d_un.d_val;
				break;
			// --- Hash Table --
			case DT_HASH:
				pBuckets = (void*)(intptr_t) dynTab[j].d_un.d_val;
				break;
			}
		}
	}

	nbuckets = pBuckets[0];
//	iSymCount = pBuckets[1];
	pBuckets = &pBuckets[2];
	pChains = &pBuckets[ nbuckets ];
	
	// Get hash
	iNameHash = ElfHashString(Name);
	iNameHash %= nbuckets;

	// Walk Chain
	i = pBuckets[ iNameHash ];
	if(symtab[i].st_shndx != SHN_UNDEF && strcmp(dynstrtab + symtab[i].st_name, Name) == 0) {
		*Ret = (void*)( (intptr_t)symtab[i].st_value + iBaseDiff );
		if(Size)	*Size = symtab[i].st_size;
		TRACE("%s = %p", Name, *Ret);
		return 1;
	}
	
	while(pChains[i] != STN_UNDEF)
	{
		i = pChains[i];
		if(symtab[i].st_shndx != SHN_UNDEF && strcmp(dynstrtab + symtab[i].st_name, Name) == 0) {
			*Ret = (void*)((intptr_t)symtab[i].st_value + iBaseDiff);
			if(Size)	*Size = symtab[i].st_size;
			TRACE("%s = %p", Name, *Ret);
			return 1;
		}
	}
	
	return 0;
}
#endif
#endif

uint32_t ElfHashString(const char *name)
{
	uint32_t	h = 0, g;
	while(*name)
	{
		h = (h << 4) + *(uint8_t*)name++;
		if( (g = h & 0xf0000000) )
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

