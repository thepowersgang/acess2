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

typedef struct
{
	void	*Base;
	intptr_t	iBaseDiff;
	const char	*strtab;
	Elf32_Sym	*symtab;
} tElfRelocInfo;

typedef int tElf32RelocFcn(tElfRelocInfo *Info, uint32_t t_info, uint32_t *ptr, Elf32_Addr addend, int bRela);

// === CONSTANTS ===
#if DEBUG
//static const char	*csaDT_NAMES[] = {"DT_NULL", "DT_NEEDED", "DT_PLTRELSZ", "DT_PLTGOT", "DT_HASH", "DT_STRTAB", "DT_SYMTAB", "DT_RELA", "DT_RELASZ", "DT_RELAENT", "DT_STRSZ", "DT_SYMENT", "DT_INIT", "DT_FINI", "DT_SONAME", "DT_RPATH", "DT_SYMBOLIC", "DT_REL", "DT_RELSZ", "DT_RELENT", "DT_PLTREL", "DT_DEBUG", "DT_TEXTREL", "DT_JMPREL"};
//static const char	*csaR_NAMES[] = {"R_386_NONE", "R_386_32", "R_386_PC32", "R_386_GOT32", "R_386_PLT32", "R_386_COPY", "R_386_GLOB_DAT", "R_386_JMP_SLOT", "R_386_RELATIVE", "R_386_GOTOFF", "R_386_GOTPC", "R_386_LAST"};
#endif

// === PROTOTYPES ===
void	*ElfRelocate(void *Base, char **envp, const char *Filename);
 int	ElfGetSymbol(void *Base, const char *Name, void **Ret, size_t *Size);
void	*Elf32Relocate(void *Base, char **envp, const char *Filename);
 int	Elf32GetSymbolVars(void *Base, Elf32_Sym** symtab, Elf32_Word** pBuckets, const char **dynstrtab, uintptr_t* piBaseDiff);
 int	Elf32GetSymbolInfo(void *Base, const char *Name, void **Addr, size_t *Size, int* Section, int *Binding, int *Type);
 int	Elf32GetSymbol(void *Base, const char *Name, void **Ret, size_t *Size);
tElf32RelocFcn	elf_doRelocate_386;
tElf32RelocFcn	elf_doRelocate_arm;
tElf32RelocFcn	elf_doRelocate_unk;
#ifdef SUPPORT_ELF64
int	_Elf64DoReloc_X86_64(void *Base, const char *strtab, Elf64_Sym *symtab, Elf64_Xword r_info, void *ptr, Elf64_Sxword addend);
void	*Elf64Relocate(void *Base, char **envp, const char *Filename);
 int	Elf64GetSymbol(void *Base, const char *Name, void **Ret, size_t *Size);
#endif
 int	Elf32GetSymbolReloc(tElfRelocInfo *Info, const Elf32_Sym *Symbol, void **Ret, size_t *Size);
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
		return Elf32Relocate(Base, envp, Filename);
#ifdef SUPPORT_ELF64
	case ELFCLASS64:
		return Elf64Relocate(Base, envp, Filename);
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
		return Elf32GetSymbol(Base, Name, ret, Size);
#ifdef SUPPORT_ELF64
	case ELFCLASS64:
		return Elf64GetSymbol(Base, Name, ret, Size);
#endif
	default:
		SysDebug("ld-acess - ElfRelocate: Unknown file class %i", hdr->e_ident[4]);
		return 0;
	}
}

int elf_doRelocate_386(tElfRelocInfo *Info, uint32_t r_info, uint32_t *ptr, Elf32_Addr addend, int bRela)
{
	const Elf32_Sym	*sym = &Info->symtab[ ELF32_R_SYM(r_info) ];
	void	*symval = (void*)sym->st_value;
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

int elf_doRelocate_arm(tElfRelocInfo *Info, uint32_t r_info, uint32_t *ptr, Elf32_Addr addend, int bRela)
{
	const Elf32_Sym	*sym = &Info->symtab[ ELF32_R_SYM(r_info) ];
	void	*symval = (void*)sym->st_value;
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

int elf_doRelocate_unk(tElfRelocInfo *Info, uint32_t r_info, uint32_t *ptr, Elf32_Addr addend, int bRela)
{
	return 1;
}

void *Elf32Relocate(void *Base, char **envp, const char *Filename)
{
	const Elf32_Ehdr	*hdr = Base;
	char	*libPath;
	intptr_t	iRealBase = -1;
	Elf32_Rel	*rel = NULL;
	Elf32_Rela	*rela = NULL;
	void	*plt = NULL;
	 int	relSz=0, relEntSz=8;
	 int	relaSz=0, relaEntSz=8;
	 int	pltSz=0, pltType=0;
	Elf32_Dyn	*dynamicTab = NULL;	// Dynamic Table Pointer
	
	TRACE("(Base=0x%x)", Base);
	
	// Check magic header
	
	
	// Parse Program Header to get Dynamic Table
	// - Determine the linked base of the executable
	const Elf32_Phdr	*phtab = (void*)( (uintptr_t)Base + hdr->phoff );
	const int iSegmentCount = hdr->phentcount;
	for(int i = 0; i < iSegmentCount; i ++)
	{
		switch(phtab[i].Type)
		{
		case PT_LOAD:
			// Determine linked base address
			if( iRealBase > phtab[i].VAddr)
				iRealBase = phtab[i].VAddr;
			break;
		case PT_DYNAMIC:
			// Find Dynamic Section
			if(!dynamicTab) {
				dynamicTab = (void *) (intptr_t) phtab[i].VAddr;
			}
			else {
				WARNING("elf_relocate: Multiple PT_DYNAMIC segments");
			}
			break;
		}
	}
	
	// Page Align real base
	iRealBase &= ~0xFFF;
	
	// Adjust "Real" Base
	const intptr_t	iBaseDiff = (intptr_t)Base - iRealBase;

	TRACE("True Base = 0x%x, Compiled Base = 0x%x, Difference = 0x%x", Base, iRealBase, iBaseDiff);

	// Check if a PT_DYNAMIC segement was found
	if(!dynamicTab) {
		SysDebug(" elf_relocate: No PT_DYNAMIC segment in image %p, returning", Base);
		return (void *)(intptr_t)(hdr->entrypoint + iBaseDiff);
	}

	// Allow writing to read-only segments, just in case they need to be relocated
	// - Will be reversed at the end of the function
	for( int i = 0; i < iSegmentCount; i ++ )
	{
		if(phtab[i].Type == PT_LOAD && !(phtab[i].Flags & PF_W) ) {
			uintptr_t	addr = phtab[i].VAddr + iBaseDiff;
			uintptr_t	end = addr + phtab[i].MemSize;
			for( ; addr < end; addr += PAGE_SIZE )
				_SysSetMemFlags(addr, 0, 1);	// Unset RO
		}
	}

	// Adjust Dynamic Table
	dynamicTab = (void *)( (intptr_t)dynamicTab + iBaseDiff );
	
	// === Get Symbol table and String Table ===
	char	*dynstrtab = NULL;	// .dynamic String Table
	Elf32_Sym	*dynsymtab = NULL;
	Elf32_Word	*hashtable = NULL;
	 int	iSymCount = 0;
	for( int j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
	{
		const Elf32_Dyn	*dt = &dynamicTab[j];
		switch(dt->d_tag)
		{
		// --- Symbol Table ---
		case DT_SYMTAB:
			TRACE("DYNAMIC Symbol Table 0x%x (0x%x)", dt->d_val, dt->d_val + iBaseDiff);
			dynsymtab = (void*)((intptr_t)dt->d_val + iBaseDiff);
			break;
		// --- String Table ---
		case DT_STRTAB:
			TRACE("DYNAMIC String Table 0x%x (0x%x)", dt->d_val, dt->d_val + iBaseDiff);
			dynstrtab = (void*)((intptr_t)dt->d_val + iBaseDiff);
			break;
		// --- Hash Table --
		case DT_HASH:
			TRACE("DYNAMIC Hash table %p (%p)", dt->d_val, dt->d_val + iBaseDiff);
			hashtable = (void*)((intptr_t)dt->d_val + iBaseDiff);
			iSymCount = hashtable[1];
			break;
		}
	}

	if(dynsymtab == NULL) {
		SysDebug("ld-acess.so - WARNING: No Dynamic Symbol table in %p, returning", hdr);
		return (void *)(intptr_t) (hdr->entrypoint + iBaseDiff);
	}
	
	// Apply base offset to locally defined symbols
	// - #0 is defined as ("" SHN_UNDEF), so skip it
	for( int i = 1; i < iSymCount; i ++ )
	{
		Elf32_Sym	*sym = &dynsymtab[i];
		const char *name = dynstrtab + sym->st_name;
		(void)name;
		if( sym->st_shndx == SHN_UNDEF )
		{
			TRACE("Sym %i'%s' deferred (SHN_UNDEF)", i, name);
		}
		else if( sym->st_shndx == SHN_ABS )
		{
			// Leave as is
			SysDebug("Sym %i'%s' untouched", i, name);
		}
		else
		{
			void *newval;
			size_t	newsize;
			if( ELF32_ST_BIND(sym->st_info) != STB_WEAK )
			{
				TRACE("Sym %i'%s' = %p (local)", i, name, sym->st_value + iBaseDiff);
				sym->st_value += iBaseDiff;
			}
			// If GetSymbol doesn't return a strong/global symbol value
			else if( GetSymbol(name, &newval, &newsize, Base) != 1 )
			{
				TRACE("Sym %i'%s' = %p (Local weak)", i, name, sym->st_value + iBaseDiff);
				sym->st_value += iBaseDiff;
			}
			else
			{
				TRACE("Sym %i'%s' = %p+0x%x (Extern weak)", i, name, newval, newsize);
				sym->st_value = (uintptr_t)newval;
				sym->st_size = newsize;
			}
		}
	}

	// === Add to loaded list (can be imported now) ===
	AddLoaded( Filename, Base );

	// === Parse Relocation Data ===
	TRACE("dynamicTab = 0x%x", dynamicTab);
	for( int j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
	{
		const Elf32_Dyn	*dt = &dynamicTab[j];
		switch(dt->d_tag)
		{
		// --- Shared Library Name ---
		case DT_SONAME:
			TRACE(".so Name '%s'", dynstrtab + dt->d_val);
			break;
		// --- Needed Library ---
		case DT_NEEDED:
			//assert(dt->d_val <= sizeof_dynstrtab);
			libPath = dynstrtab + dt->d_val;
			TRACE(" Required Library '%s'", libPath);
			if(LoadLibrary(libPath, NULL, envp) == 0) {
				SysDebug("Unable to load required library '%s'", libPath);
				return 0;
			}
			TRACE(" Lib loaded");
			break;
		// --- PLT/GOT ---
//		case DT_PLTGOT:	pltgot = (void*)(iBaseDiff + dt->d_val);	break;
		case DT_JMPREL:	plt = (void*)(iBaseDiff + dt->d_val);	break;
		case DT_PLTREL:	pltType = dt->d_val;	break;
		case DT_PLTRELSZ:	pltSz = dt->d_val;	break;
		
		// --- Relocation ---
		case DT_REL:	rel = (void*)(iBaseDiff + dt->d_val);	break;
		case DT_RELSZ:	relSz = dt->d_val;	break;
		case DT_RELENT:	relEntSz = dt->d_val;	break;
		case DT_RELA:	rela = (void*)(iBaseDiff + dt->d_val);	break;
		case DT_RELASZ:	relaSz = dt->d_val;	break;
		case DT_RELAENT:	relaEntSz = dt->d_val;	break;
		
		// --- Symbol Table ---
		case DT_SYMTAB:
		// --- Hash Table ---
		case DT_HASH:
		// --- String Table ---
		case DT_STRTAB:
			break;
		
		// --- Unknown ---
		default:
			if(dt->d_tag > DT_JMPREL)	continue;
			//DEBUGS(" elf_relocate: %i-%i = %s,0x%x",
			//	i,j, csaDT_NAMES[dynamicTab[j].d_tag],dynamicTab[j].d_val);
			break;
		}
	}
	
	// Resolve symbols (second pass)
	// - #0 is defined as ("" SHN_UNDEF), so skip it
	 int	fail = 0;
	for( int i = 1; i < iSymCount; i ++ )
	{
		Elf32_Sym	*sym = &dynsymtab[i];
		const char *name = dynstrtab + sym->st_name;
		if( sym->st_shndx == SHN_UNDEF )
		{
			void *newval;
			size_t	newsize;
			if( !GetSymbol(name, &newval, &newsize, Base) ) {
				if( ELF32_ST_BIND(sym->st_info) != STB_WEAK ) {
					// Not a weak binding, set fail and move on
					WARNING("Elf32Relocate: Can't find required symbol '%s' for '%s'",
						name, Filename);
					fail = 1;
					continue ;
				}
				// Leave the symbol value as-is
			}
			else {
				TRACE("Sym %i'%s' bound to %p+0x%x", i, name, newval, newsize);
				sym->st_value = (intptr_t)newval;
				sym->st_size = newsize;
			}
		}
		else if( sym->st_shndx == SHN_ABS )
		{
			// Leave as is
		}
		else
		{
			// Handled previously
			// TODO: What about weak locally-defined symbols?
			//assert( ELF32_ST_BIND(sym->st_info) != STB_WEAK );
		}
	}
	if( fail ) {
		WARNING("Relocation of '%s' failed", Filename);
		return NULL;
	}
	
	TRACE("Beginning Relocation on '%s'", Filename);


	tElf32RelocFcn	*do_relocate;
	switch(hdr->machine)
	{
	case EM_386:
		do_relocate = elf_doRelocate_386;
		break;
	case EM_ARM:
		do_relocate = elf_doRelocate_arm;
		break;
	default:
		SysDebug("Elf32Relocate: Unknown machine type %i", hdr->machine);
		do_relocate = elf_doRelocate_unk;
		fail = 1;
		break;
	}
	
	TRACE("do_relocate = %p (%p or %p)", do_relocate, &elf_doRelocate_386, &elf_doRelocate_arm);

	#define _doRelocate(r_info, ptr, bRela, addend)	\
		do_relocate(&reloc_info, r_info, ptr, addend, bRela);

	tElfRelocInfo	reloc_info = {
		.Base = Base,
		.iBaseDiff = iBaseDiff,
		.strtab = dynstrtab,
		.symtab = dynsymtab
	};

	// Parse Relocation Entries
	if(rel && relSz)
	{
		Elf32_Word	*ptr;
		TRACE("rel=0x%x, relSz=0x%x, relEntSz=0x%x", rel, relSz, relEntSz);
		int max = relSz / relEntSz;
		for( int i = 0; i < max; i++ )
		{
			ptr = (void*)(iBaseDiff + rel[i].r_offset);
			fail |= _doRelocate(rel[i].r_info, ptr, 0, *ptr);
		}
	}
	// Parse Relocation Entries
	if(rela && relaSz)
	{
		Elf32_Word	*ptr;
		TRACE("rela=0x%x, relaSz=0x%x, relaEntSz=0x%x", rela, relaSz, relaEntSz);
		int count = relaSz / relaEntSz;
		for( int i = 0; i < count; i++ )
		{
			ptr = (void*)(iBaseDiff + rela[i].r_offset);
			fail |= _doRelocate(rel[i].r_info, ptr, 1, rela[i].r_addend);
		}
	}
	
	// === Process PLT (Procedure Linkage Table) ===
	if(plt && pltSz)
	{
		Elf32_Word	*ptr;
		TRACE("Relocate PLT, plt=0x%x", plt);
		if(pltType == DT_REL)
		{
			Elf32_Rel	*pltRel = plt;
			int count = pltSz / sizeof(Elf32_Rel);
			TRACE("PLT Reloc Type = Rel, %i entries", count);
			for(int i = 0; i < count; i ++)
			{
				ptr = (void*)(iBaseDiff + pltRel[i].r_offset);
				fail |= _doRelocate(pltRel[i].r_info, ptr, 0, *ptr);
			}
		}
		else
		{
			Elf32_Rela	*pltRela = plt;
			int count = pltSz / sizeof(Elf32_Rela);
			TRACE("PLT Reloc Type = Rela, %i entries", count);
			for(int i=0;i<count;i++)
			{
				ptr = (void*)(iRealBase + pltRela[i].r_offset);
				fail |= _doRelocate(pltRela[i].r_info, ptr, 1, pltRela[i].r_addend);
			}
		}
	}

	// Re-set readonly
	for( int i = 0; i < iSegmentCount; i ++ )
	{
		// If load and not writable
		if(phtab[i].Type == PT_LOAD && !(phtab[i].Flags & PF_W) ) {
			uintptr_t	addr = phtab[i].VAddr + iBaseDiff;
			uintptr_t	end = addr + phtab[i].MemSize;
			for( ; addr < end; addr += PAGE_SIZE )
				_SysSetMemFlags(addr, 1, 1);	// Unset RO
		}
	}

	if( fail ) {
		TRACE("ElfRelocate: Failure");
		return NULL;
	}	

	#undef _doRelocate

	TRACE("RETURN 0x%x to %p", hdr->entrypoint + iBaseDiff, __builtin_return_address(0));
	return (void*)(intptr_t)( hdr->entrypoint + iBaseDiff );
}

int Elf32GetSymbolVars(void *Base, Elf32_Sym** symtab, Elf32_Word** pBuckets, const char **dynstrtab, uintptr_t* piBaseDiff)
{
	Elf32_Dyn	*dynTab = NULL;
	uintptr_t	iBaseDiff = -1;
	Elf32_Ehdr *hdr = Base;
	Elf32_Phdr *phtab = (void*)( (uintptr_t)Base + hdr->phoff );
	for( int i = 0; i < hdr->phentcount; i ++ )
	{
		if(phtab[i].Type == PT_LOAD && iBaseDiff > phtab[i].VAddr)
			iBaseDiff = phtab[i].VAddr;
		if( phtab[i].Type == PT_DYNAMIC ) {
			dynTab = (void*)(intptr_t)phtab[i].VAddr;
		}
	}
	if( !dynTab ) {
		SysDebug("ERROR - Unable to find DYNAMIC segment in %p", Base);
		return 1;
	}
	iBaseDiff = (intptr_t)Base - iBaseDiff;	// Make iBaseDiff actually the diff
	dynTab = (void*)( (intptr_t)dynTab + iBaseDiff );
	for( int i = 0; dynTab[i].d_tag != DT_NULL; i++)
	{
		switch(dynTab[i].d_tag)
		{
		// --- Symbol Table ---
		case DT_SYMTAB:
			*symtab = (void*)((intptr_t)dynTab[i].d_val + iBaseDiff);	// Rebased in Relocate
			break;
		case DT_STRTAB:
			*dynstrtab = (void*)((intptr_t)dynTab[i].d_val + iBaseDiff);
			break;
		// --- Hash Table --
		case DT_HASH:
			*pBuckets = (void*)((intptr_t)dynTab[i].d_val + iBaseDiff);
			break;
		}
	}
	
	if( !*symtab ) {
		SysDebug("ERRO - No DT_SYMTAB in %p", Base);
		return 1;
	}
	if( !*pBuckets ) {
		SysDebug("ERRO - No DT_HASH in %p", Base);
		return 1;
	}
	if( !*dynstrtab ) {
		SysDebug("ERRO - No DT_STRTAB in %p", Base);
		return 1;
	}

	// ... ok... maybe they haven't been relocated
	if( (uintptr_t)*symtab < (uintptr_t)Base )
	{
		SysDebug("Executable not yet relocated (symtab,pBuckets,dynstrtab = %p,%p,%p + 0x%x)",
			*symtab,*pBuckets,*dynstrtab, iBaseDiff);
		*symtab    = (void*)( (uintptr_t)*symtab    + iBaseDiff );
		*pBuckets  = (void*)( (uintptr_t)*pBuckets  + iBaseDiff );
		*dynstrtab = (void*)( (uintptr_t)*dynstrtab + iBaseDiff );
	}
	*piBaseDiff = iBaseDiff;
	return 0;
}

int Elf32GetSymbolInfo(void *Base, const char *Name, void **Addr, size_t *Size, int* Section, int *Binding, int *Type)
{
	// Locate the tables
	uintptr_t	iBaseDiff = -1;
	Elf32_Sym	*symtab = NULL;
	Elf32_Word	*pBuckets = NULL;
	const char	*dynstrtab = NULL;
	if( Elf32GetSymbolVars(Base, &symtab, &pBuckets, &dynstrtab, &iBaseDiff) )
		return 1;

	int nbuckets = pBuckets[0];
//	int iSymCount = pBuckets[1];
	pBuckets = &pBuckets[2];
	Elf32_Word* pChains = &pBuckets[ nbuckets ];
	assert(pChains);

	// Get hash
	int iNameHash = ElfHashString(Name);
	iNameHash %= nbuckets;

	// Walk Chain
	int idx = pBuckets[ iNameHash ];
	do {
		const Elf32_Sym *sym = &symtab[idx];
		assert(sym);
		if( strcmp(dynstrtab + sym->st_name, Name) == 0 )
		{
			TRACE("*sym = {value:0x%x,size:0x%x,info:0x%x,other:0x%x,shndx:%i}",
				sym->st_value, sym->st_size, sym->st_info,
				sym->st_other, sym->st_shndx);
			if(Addr)	*Addr = (void*)( sym->st_value );
			if(Size)	*Size = sym->st_size;
			if(Binding)	*Binding = ELF32_ST_BIND(sym->st_info);
			if(Type)	*Type = ELF32_ST_TYPE(sym->st_info);
			if(Section)	*Section = sym->st_shndx;
			return 0;
		}
	} while( (idx = pChains[idx]) != STN_UNDEF && idx != pBuckets[iNameHash] );
	
	TRACE("No symbol");
	return 1;
}

int Elf32GetSymbol(void *Base, const char *Name, void **ret, size_t *Size)
{
	 int	section, binding;
	TRACE("Elf32GetSymbol(%p,%s,...)", Base, Name);
	if( Elf32GetSymbolInfo(Base, Name, ret, Size, &section, &binding, NULL) )
		return 0;
	if( section == SHN_UNDEF ) {
		TRACE("Elf32GetSymbol: Undefined %p", *ret, (Size?*Size:0), section);
		return 0;
	}
	if( binding == STB_WEAK ) {
		TRACE("Elf32GetSymbol: Weak, return %p+0x%x,section=%i", *ret, (Size?*Size:0), section);
		return 2;
	}
	TRACE("Elf32GetSymbol: Found %p+0x%x,section=%i", *ret, (Size?*Size:0), section);
	return 1;
}

#ifdef SUPPORT_ELF64
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

