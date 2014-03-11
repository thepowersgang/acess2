/*
 * Acess2 Dynamic Linker
 * - By John Hodge (thePowersGang)
 *
 * elf.c
 * - ELF32/ELF64 relocation
 */
#ifndef DEBUG	// This code is #include'd from the kernel, so DEBUG may already be defined
# define DEBUG	0
#endif

#ifndef PAGE_SIZE
# define PAGE_SIZE	4096
#endif

#include "common.h"
#include <stdint.h>
#include <assert.h>
#include "elf32.h"
#include "elf64.h"

#if DEBUG
# define	DEBUGS(v...)	SysDebug("ld-acess - " v)
#else
# define	DEBUGS(...)	
#endif

#ifndef DISABLE_ELF64
# define SUPPORT_ELF64
#endif

// === CONSTANTS ===
#if DEBUG
//static const char	*csaDT_NAMES[] = {"DT_NULL", "DT_NEEDED", "DT_PLTRELSZ", "DT_PLTGOT", "DT_HASH", "DT_STRTAB", "DT_SYMTAB", "DT_RELA", "DT_RELASZ", "DT_RELAENT", "DT_STRSZ", "DT_SYMENT", "DT_INIT", "DT_FINI", "DT_SONAME", "DT_RPATH", "DT_SYMBOLIC", "DT_REL", "DT_RELSZ", "DT_RELENT", "DT_PLTREL", "DT_DEBUG", "DT_TEXTREL", "DT_JMPREL"};
static const char	*csaR_NAMES[] = {"R_386_NONE", "R_386_32", "R_386_PC32", "R_386_GOT32", "R_386_PLT32", "R_386_COPY", "R_386_GLOB_DAT", "R_386_JMP_SLOT", "R_386_RELATIVE", "R_386_GOTOFF", "R_386_GOTPC", "R_386_LAST"};
#endif

// === PROTOTYPES ===
void	*ElfRelocate(void *Base, char **envp, const char *Filename);
 int	ElfGetSymbol(void *Base, const char *Name, void **Ret, size_t *Size);
void	*Elf32Relocate(void *Base, char **envp, const char *Filename);
 int	Elf32GetSymbol(void *Base, const char *Name, void **Ret, size_t *Size);
 int	elf_doRelocate_386(uint32_t r_info, uint32_t *ptr, Elf32_Addr addend, int type, int bRela, const char *Sym, intptr_t iBaseDiff);
 int	elf_doRelocate_arm(uint32_t r_info, uint32_t *ptr, Elf32_Addr addend, int type, int bRela, const char *Sym, intptr_t iBaseDiff);
 int	elf_doRelocate_unk(uint32_t , uint32_t *, Elf32_Addr , int , int , const char *, intptr_t);
#ifdef SUPPORT_ELF64
int	_Elf64DoReloc_X86_64(void *Base, const char *strtab, Elf64_Sym *symtab, Elf64_Xword r_info, void *ptr, Elf64_Sxword addend);
void	*Elf64Relocate(void *Base, char **envp, const char *Filename);
 int	Elf64GetSymbol(void *Base, const char *Name, void **Ret, size_t *Size);
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

int elf_doRelocate_386(uint32_t r_info, uint32_t *ptr, Elf32_Addr addend, int type,
		int bRela, const char *Sym, intptr_t iBaseDiff)
{
	void	*symval;
	switch( type )
	{
	// Standard 32 Bit Relocation (S+A)
	case R_386_32:
		if( !GetSymbol(Sym, &symval, NULL) )
			return 1;
		DEBUGS(" elf_doRelocate: R_386_32 *0x%x += %p('%s')",
				ptr, symval, Sym);
		*ptr = (intptr_t)symval + addend;
		break;
		
	// 32 Bit Relocation wrt. Offset (S+A-P)
	case R_386_PC32:
		DEBUGS(" elf_doRelocate: '%s'", Sym);
		if( !GetSymbol(Sym, &symval, NULL) )	return 1;
		DEBUGS(" elf_doRelocate: R_386_PC32 *0x%x = 0x%x + 0x%p - 0x%x",
			ptr, *ptr, symval, (intptr_t)ptr );
		*ptr = (intptr_t)symval + addend - (intptr_t)ptr;
		//*ptr = val + addend - ((Uint)ptr - iBaseDiff);
		break;

	// Absolute Value of a symbol (S)
	case R_386_GLOB_DAT:
	case R_386_JMP_SLOT:
		DEBUGS(" elf_doRelocate: '%s'", Sym);
		if( !GetSymbol(Sym, &symval, NULL) )	return 1;
		DEBUGS(" elf_doRelocate: %s *0x%x = %p", csaR_NAMES[type], ptr, symval);
		*ptr = (intptr_t)symval;
		break;

	// Base Address (B+A)
	case R_386_RELATIVE:
		DEBUGS(" elf_doRelocate: R_386_RELATIVE *0x%x = 0x%x + 0x%x", ptr, iBaseDiff, addend);
		*ptr = iBaseDiff + addend;
		break;

	case R_386_COPY: {
		size_t	size;
		if( !GetSymbol(Sym, &symval, &size) )	return 1;
		DEBUGS(" elf_doRelocate_386: R_386_COPY (%p, %p, %i)", ptr, symval, size);
		memcpy(ptr, symval, size);
		break; }

	default:
		SysDebug("elf_doRelocate_386: Unknown relocation %i", type);
		return 2;
	}
	return 0;
}

int elf_doRelocate_arm(uint32_t r_info, uint32_t *ptr, Elf32_Addr addend, int type, int bRela, const char *Sym, intptr_t iBaseDiff)
{
	uint32_t	val;
	switch(type)
	{
	// (S + A) | T
	case R_ARM_ABS32:
		DEBUGS(" elf_doRelocate_arm: R_ARM_ABS32 %p (%s + %x)", ptr, Sym, addend);
		if( !GetSymbol(Sym, (void**)&val, NULL) )	return 1;
		*ptr = val + addend;
		break;
	case R_ARM_GLOB_DAT:
		DEBUGS(" elf_doRelocate_arm: R_ARM_GLOB_DAT %p (%s + %x)", ptr, Sym, addend);
		if( !GetSymbol(Sym, (void**)&val, NULL) )	return 1;
		*ptr = val + addend;
		break;
	case R_ARM_JUMP_SLOT:
		if(!bRela)	addend = 0;
		DEBUGS(" elf_doRelocate_arm: R_ARM_JUMP_SLOT %p (%s + %x)", ptr, Sym, addend);
		if( !GetSymbol(Sym, (void**)&val, NULL) )	return 1;
		*ptr = val + addend;
		break;
	// Copy
	case R_ARM_COPY: {
		size_t	size;
		void	*src;
		if( !GetSymbol(Sym, &src, &size) )	return 1;
		DEBUGS(" elf_doRelocate_arm: R_ARM_COPY (%p, %p, %i)", ptr, src, size);
		memcpy(ptr, src, size);
		break; }
	// Delta between link and runtime locations + A
	case R_ARM_RELATIVE:
		DEBUGS(" elf_doRelocate_arm: R_ARM_RELATIVE %p (0x%x + 0x%x)", ptr, iBaseDiff, addend);
		if(Sym[0] != '\0') {
			// TODO: Get delta for a symbol
			SysDebug("elf_doRelocate_arm: TODO - Implment R_ARM_RELATIVE for symbols");
			return 2;
		}
		else {
			*ptr = iBaseDiff + addend;
		}
		break;
	default:
		SysDebug("elf_doRelocate_arm: Unknown Relocation, %i", type);
		return 2;
	}
	return 0;
}

int elf_doRelocate_unk(uint32_t r_info, uint32_t *ptr, Elf32_Addr addend, int type, int bRela, const char *Sym, intptr_t iBaseDiff)
{
	return 1;
}

void *Elf32Relocate(void *Base, char **envp, const char *Filename)
{
	Elf32_Ehdr	*hdr = Base;
	Elf32_Phdr	*phtab;
	char	*libPath;
	intptr_t	iRealBase = -1;
	intptr_t	iBaseDiff;
	 int	iSegmentCount;
	Elf32_Rel	*rel = NULL;
	Elf32_Rela	*rela = NULL;
	void	*plt = NULL;
	 int	relSz=0, relEntSz=8;
	 int	relaSz=0, relaEntSz=8;
	 int	pltSz=0, pltType=0;
	Elf32_Dyn	*dynamicTab = NULL;	// Dynamic Table Pointer
	char	*dynstrtab = NULL;	// .dynamic String Table
	Elf32_Sym	*dynsymtab;
	int	(*do_relocate)(uint32_t t_info, uint32_t *ptr, Elf32_Addr addend, int Type, int bRela, const char *Sym, intptr_t iBaseDiff);
	
	DEBUGS("ElfRelocate: (Base=0x%x)", Base);
	
	// Check magic header
	
	
	// Parse Program Header to get Dynamic Table
	phtab = (void*)( (uintptr_t)Base + hdr->phoff );
	iSegmentCount = hdr->phentcount;
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
				DEBUGS(" WARNING - elf_relocate: Multiple PT_DYNAMIC segments");
			}
			break;
		}
	}
	
	// Page Align real base
	iRealBase &= ~0xFFF;
	DEBUGS(" elf_relocate: True Base = 0x%x, Compiled Base = 0x%x", Base, iRealBase);
	
	// Adjust "Real" Base
	iBaseDiff = (intptr_t)Base - iRealBase;

//	hdr->entrypoint += iBaseDiff;	// Adjust Entrypoint
	
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
	dynsymtab = NULL;
	for( int j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
	{
		switch(dynamicTab[j].d_tag)
		{
		// --- Symbol Table ---
		case DT_SYMTAB:
			DEBUGS(" elf_relocate: DYNAMIC Symbol Table 0x%x (0x%x)",
				dynamicTab[j].d_val, dynamicTab[j].d_val + iBaseDiff);
			dynsymtab = (void*)((intptr_t)dynamicTab[j].d_val + iBaseDiff);
			//if(iBaseDiff != 0)	dynamicTab[j].d_val += iBaseDiff;
			break;
		// --- String Table ---
		case DT_STRTAB:
			DEBUGS(" elf_relocate: DYNAMIC String Table 0x%x (0x%x)",
				dynamicTab[j].d_val, dynamicTab[j].d_val + iBaseDiff);
			dynstrtab = (void*)((intptr_t)dynamicTab[j].d_val + iBaseDiff);
			//if(iBaseDiff != 0)	dynamicTab[j].d_val += iBaseDiff;
			break;
		// --- Hash Table --
		case DT_HASH:
			//if(iBaseDiff != 0)	dynamicTab[j].d_val += iBaseDiff;
//			iSymCount = ((Elf32_Word*)(intptr_t)dynamicTab[j].d_val)[1];
			break;
		}
	}

	if(dynsymtab == NULL) {
		SysDebug("ld-acess.so - WARNING: No Dynamic Symbol table in %p, returning", hdr);
		return (void *)(intptr_t) (hdr->entrypoint + iBaseDiff);
	}

	// === Add to loaded list (can be imported now) ===
	AddLoaded( Filename, Base );

	// === Parse Relocation Data ===
	DEBUGS(" elf_relocate: dynamicTab = 0x%x", dynamicTab);
	for( int j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
	{
		switch(dynamicTab[j].d_tag)
		{
		// --- Shared Library Name ---
		case DT_SONAME:
			DEBUGS(" elf_relocate: .so Name '%s'", dynstrtab+dynamicTab[j].d_val);
			break;
		// --- Needed Library ---
		case DT_NEEDED:
			libPath = dynstrtab + dynamicTab[j].d_val;
			DEBUGS(" dynstrtab = %p, d_val = 0x%x", dynstrtab, dynamicTab[j].d_val);
			DEBUGS(" Required Library '%s'", libPath);
			if(LoadLibrary(libPath, NULL, envp) == 0) {
				#if DEBUG
				DEBUGS(" elf_relocate: Unable to load '%s'", libPath);
				#else
				SysDebug("Unable to load required library '%s'", libPath);
				#endif
				return 0;
			}
			DEBUGS(" Lib loaded");
			break;
		// --- PLT/GOT ---
//		case DT_PLTGOT:	pltgot = (void*)(iBaseDiff + dynamicTab[j].d_val);	break;
		case DT_JMPREL:	plt = (void*)(iBaseDiff + dynamicTab[j].d_val);	break;
		case DT_PLTREL:	pltType = dynamicTab[j].d_val;	break;
		case DT_PLTRELSZ:	pltSz = dynamicTab[j].d_val;	break;
		
		// --- Relocation ---
		case DT_REL:	rel = (void*)(iBaseDiff + dynamicTab[j].d_val);	break;
		case DT_RELSZ:	relSz = dynamicTab[j].d_val;	break;
		case DT_RELENT:	relEntSz = dynamicTab[j].d_val;	break;
		case DT_RELA:	rela = (void*)(iBaseDiff + dynamicTab[j].d_val);	break;
		case DT_RELASZ:	relaSz = dynamicTab[j].d_val;	break;
		case DT_RELAENT:	relaEntSz = dynamicTab[j].d_val;	break;
		
		// --- Symbol Table ---
		case DT_SYMTAB:
		// --- Hash Table ---
		case DT_HASH:
		// --- String Table ---
		case DT_STRTAB:
			break;
		
		// --- Unknown ---
		default:
			if(dynamicTab[j].d_tag > DT_JMPREL)	continue;
			//DEBUGS(" elf_relocate: %i-%i = %s,0x%x",
			//	i,j, csaDT_NAMES[dynamicTab[j].d_tag],dynamicTab[j].d_val);
			break;
		}
	}
	
	DEBUGS(" elf_relocate: Beginning Relocation");

	 int	fail = 0;

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
	
	DEBUGS("do_relocate = %p (%p or %p)", do_relocate, &elf_doRelocate_386, &elf_doRelocate_arm);

	#define _doRelocate(r_info, ptr, bRela, addend)	\
		do_relocate(r_info, ptr, addend, ELF32_R_TYPE(r_info), bRela, \
			dynstrtab + dynsymtab[ELF32_R_SYM(r_info)].nameOfs, iBaseDiff);

	// Parse Relocation Entries
	if(rel && relSz)
	{
		Elf32_Word	*ptr;
		DEBUGS(" elf_relocate: rel=0x%x, relSz=0x%x, relEntSz=0x%x", rel, relSz, relEntSz);
		int max = relSz / relEntSz;
		for( int i = 0; i < max; i++ )
		{
			//DEBUGS("  Rel %i: 0x%x+0x%x", i, iBaseDiff, rel[i].r_offset);
			ptr = (void*)(iBaseDiff + rel[i].r_offset);
			fail |= _doRelocate(rel[i].r_info, ptr, 0, *ptr);
		}
	}
	// Parse Relocation Entries
	if(rela && relaSz)
	{
		Elf32_Word	*ptr;
		DEBUGS(" elf_relocate: rela=0x%x, relaSz=0x%x, relaEntSz=0x%x", rela, relaSz, relaEntSz);
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
		DEBUGS(" elf_relocate: Relocate PLT, plt=0x%x", plt);
		if(pltType == DT_REL)
		{
			Elf32_Rel	*pltRel = plt;
			int count = pltSz / sizeof(Elf32_Rel);
			DEBUGS(" elf_relocate: PLT Reloc Type = Rel, %i entries", count);
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
			DEBUGS(" elf_relocate: PLT Reloc Type = Rela, %i entries", count);
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
		DEBUGS("ElfRelocate: Failure");
		return NULL;
	}	

	#undef _doRelocate

	DEBUGS("ElfRelocate: RETURN 0x%x to %p", hdr->entrypoint + iBaseDiff, __builtin_return_address(0));
	return (void*)(intptr_t)( hdr->entrypoint + iBaseDiff );
}

int Elf32GetSymbol(void *Base, const char *Name, void **ret, size_t *Size)
{
	Elf32_Ehdr	*hdr = Base;
	Elf32_Sym	*symtab = NULL;
	 int	nbuckets = 0;
	Elf32_Word	*pBuckets = NULL;
	Elf32_Word	*pChains;
	uint32_t	iNameHash;
	const char	*dynstrtab = NULL;
	uintptr_t	iBaseDiff = -1;
	Elf32_Phdr	*phtab;
	Elf32_Dyn	*dynTab = NULL;

	// Locate the tables
	phtab = (void*)( (uintptr_t)Base + hdr->phoff );
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
		return 0;
	}
	iBaseDiff = (intptr_t)Base - iBaseDiff;	// Make iBaseDiff actually the diff
	dynTab = (void*)( (intptr_t)dynTab + iBaseDiff );
	for( int i = 0; dynTab[i].d_tag != DT_NULL; i++)
	{
		switch(dynTab[i].d_tag)
		{
		// --- Symbol Table ---
		case DT_SYMTAB:
			symtab = (void*)((intptr_t)dynTab[i].d_val + iBaseDiff);	// Rebased in Relocate
			break;
		case DT_STRTAB:
			dynstrtab = (void*)((intptr_t)dynTab[i].d_val + iBaseDiff);
			break;
		// --- Hash Table --
		case DT_HASH:
			pBuckets = (void*)((intptr_t)dynTab[i].d_val + iBaseDiff);
			break;
		}
	}
	
	if( !symtab ) {
		SysDebug("ERRO - No DT_SYMTAB in %p", Base);
		return 0;
	}
	if( !pBuckets ) {
		SysDebug("ERRO - No DT_HASH in %p", Base);
		return 0;
	}
	if( !dynstrtab ) {
		SysDebug("ERRO - No DT_STRTAB in %p", Base);
		return 0;
	}

	// ... ok... maybe they haven't been relocated
	if( (uintptr_t)symtab < (uintptr_t)Base )
	{
		symtab    = (void*)( (uintptr_t)symtab    + iBaseDiff );
		pBuckets  = (void*)( (uintptr_t)pBuckets  + iBaseDiff );
		dynstrtab = (void*)( (uintptr_t)dynstrtab + iBaseDiff );
		SysDebug("Executable not yet relocated");
	}

	nbuckets = pBuckets[0];
//	iSymCount = pBuckets[1];
	pBuckets = &pBuckets[2];
	pChains = &pBuckets[ nbuckets ];
	assert(pChains);

	// Get hash
	iNameHash = ElfHashString(Name);
	iNameHash %= nbuckets;

	// Walk Chain
	int idx = pBuckets[ iNameHash ];
	do {
		Elf32_Sym *sym = &symtab[idx];
		assert(sym);
		if(sym->shndx != SHN_UNDEF && strcmp(dynstrtab + sym->nameOfs, Name) == 0) {
			*ret = (void*)( (uintptr_t)sym->value + iBaseDiff );
			if(Size)	*Size = sym->size;
			return 1;
		}
	} while( (idx = pChains[idx]) != STN_UNDEF && idx != pBuckets[iNameHash] );
	
	return 0;
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
		if( !GetSymbol(symname, &symval, NULL)  )	return 1;
		*(uint64_t*)ptr = (uintptr_t)symval + addend;
		break;
	case R_X86_64_COPY: {
		size_t	size;
		if( !GetSymbol(symname, &symval, &size)  )	return 1;
		memcpy(ptr, symval, size);
		} break;
	case R_X86_64_GLOB_DAT:
		if( !GetSymbol(symname, &symval, NULL)  )	return 1;
		*(uint64_t*)ptr = (uintptr_t)symval;
		break;
	case R_X86_64_JUMP_SLOT:
		if( !GetSymbol(symname, &symval, NULL)  )	return 1;
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

	DEBUGS("Elf64Relocate: hdr = {");
	DEBUGS("Elf64Relocate:  e_ident = '%.16s'", hdr->e_ident);
	DEBUGS("Elf64Relocate:  e_type = 0x%x", hdr->e_type);
	DEBUGS("Elf64Relocate:  e_machine = 0x%x", hdr->e_machine);
	DEBUGS("Elf64Relocate:  e_version = 0x%x", hdr->e_version);
	DEBUGS("Elf64Relocate:  e_entry = %p", hdr->e_entry);
	DEBUGS("Elf64Relocate:  e_phoff = 0x%llx", hdr->e_phoff);
	DEBUGS("Elf64Relocate:  e_shoff = 0x%llx", hdr->e_shoff);
	DEBUGS("Elf64Relocate:  e_flags = 0x%x", hdr->e_flags);
	DEBUGS("Elf64Relocate:  e_ehsize = 0x%x", hdr->e_ehsize);
	DEBUGS("Elf64Relocate:  e_phentsize = 0x%x", hdr->e_phentsize);
	DEBUGS("Elf64Relocate:  e_phnum = %i", hdr->e_phnum);

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

	DEBUGS("baseDiff = %p", baseDiff);

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
		DEBUGS("dyntab[%i].d_tag = %i", i, dyntab[i].d_tag);
		switch(dyntab[i].d_tag)
		{
		case DT_SONAME:	break;

		case DT_NEEDED: {
			char *libPath = strtab + dyntab[i].d_un.d_val;
			DEBUGS("Elf64Relocate: libPath = '%s'", libPath);
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

	// Relocation function
	t_elf64_doreloc fpElf64DoReloc = &_Elf64DoReloc_X86_64;
	#define _Elf64DoReloc(info, ptr, addend)	fpElf64DoReloc(Base, strtab, symtab, info, ptr, addend)

	int fail = 0;
	if( rel )
	{
		DEBUGS("rel_count = %i", rel_count);
		for( i = 0; i < rel_count; i ++ )
		{
			uint64_t *ptr = (void *)(uintptr_t)( rel[i].r_offset + baseDiff );
			fail |= _Elf64DoReloc( rel[i].r_info, ptr, *ptr);
		}
	}

	if( rela )
	{
		DEBUGS("rela_count = %i", rela_count);
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
			DEBUGS("plt rel count = %i", count);
			for( i = 0; i < count; i ++ )
			{
				uint64_t *ptr = (void *)(uintptr_t)( plt[i].r_offset + baseDiff );
				fail |= _Elf64DoReloc( plt[i].r_info, ptr, *ptr);
			}
		}
		else {
			Elf64_Rela	*plt = pltrel;
			 int	count = plt_size / sizeof(Elf64_Rela);
			DEBUGS("plt rela count = %i", count);
			for( i = 0; i < count; i ++ )
			{
				uint64_t *ptr = (void *)(uintptr_t)( plt[i].r_offset + baseDiff );
				fail |= _Elf64DoReloc( plt[i].r_info, ptr, plt[i].r_addend);
			}
		}
	}

	if( fail ) {
		DEBUGS("Elf64Relocate: Failure");
		return NULL;
	}

	{
	void *ret = (void *)(uintptr_t)(hdr->e_entry + baseDiff);
	DEBUGS("Elf64Relocate: Relocations done, return %p", ret);
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
		DEBUGS("%s = %p", Name, *Ret);
		return 1;
	}
	
	while(pChains[i] != STN_UNDEF)
	{
		i = pChains[i];
		if(symtab[i].st_shndx != SHN_UNDEF && strcmp(dynstrtab + symtab[i].st_name, Name) == 0) {
			*Ret = (void*)((intptr_t)symtab[i].st_value + iBaseDiff);
			if(Size)	*Size = symtab[i].st_size;
			DEBUGS("%s = %p", Name, *Ret);
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

