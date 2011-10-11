/*
 * AcessOS 1 - Dynamic Loader
 * By thePowersGang
 */
#include "common.h"
#include <stdint.h>
#include "elf32.h"
#include "elf64.h"

#define DEBUG	0

#if DEBUG
# define	DEBUGS(v...)	SysDebug("ld-acess - " v)
#else
# define	DEBUGS(...)	
#endif

// === CONSTANTS ===
#if DEBUG
//static const char	*csaDT_NAMES[] = {"DT_NULL", "DT_NEEDED", "DT_PLTRELSZ", "DT_PLTGOT", "DT_HASH", "DT_STRTAB", "DT_SYMTAB", "DT_RELA", "DT_RELASZ", "DT_RELAENT", "DT_STRSZ", "DT_SYMENT", "DT_INIT", "DT_FINI", "DT_SONAME", "DT_RPATH", "DT_SYMBOLIC", "DT_REL", "DT_RELSZ", "DT_RELENT", "DT_PLTREL", "DT_DEBUG", "DT_TEXTREL", "DT_JMPREL"};
static const char	*csaR_NAMES[] = {"R_386_NONE", "R_386_32", "R_386_PC32", "R_386_GOT32", "R_386_PLT32", "R_386_COPY", "R_386_GLOB_DAT", "R_386_JMP_SLOT", "R_386_RELATIVE", "R_386_GOTOFF", "R_386_GOTPC", "R_386_LAST"};
#endif

// === PROTOTYPES ===
void	*ElfRelocate(void *Base, char **envp, const char *Filename);
void	*Elf32Relocate(void *Base, char **envp, const char *Filename);
void	*Elf64Relocate(void *Base, char **envp, const char *Filename);
 int	ElfGetSymbol(void *Base, const char *Name, void **Ret);
 int	Elf64GetSymbol(void *Base, const char *Name, void **Ret);
 int	Elf32GetSymbol(void *Base, const char *Name, void **Ret);
Uint32	ElfHashString(const char *name);

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
	case ELFCLASS64:
		return Elf64Relocate(Base, envp, Filename);
	default:
		SysDebug("ld-acess - ElfRelocate: Unknown file class %i", hdr->e_ident[4]);
		return NULL;
	}
}

void *Elf64Relocate(void *Base, char **envp, const char *Filename)
{
	 int	i;
	Elf64_Ehdr	*hdr = Base;
	Elf64_Phdr	*phtab;
	Elf64_Dyn	*dyntab;
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
	phtab = Base + hdr->e_phoff;
	for( i = 0; i < hdr->e_phnum; i ++ )
	{
		if(phtab[i].p_type == PT_DYNAMIC)
			dyntab = (void *)(intptr_t)phtab[i].p_vaddr;
		if(phtab[i].p_type == PT_LOAD && compiledBase > phtab[i].p_vaddr)
			compiledBase = phtab[i].p_vaddr;
	}

	baseDiff = (Elf64_Addr)Base - compiledBase;

	DEBUGS("baseDiff = %p", baseDiff);

	if(dyntab == NULL) {
		SysDebug(" Elf64Relocate: No PT_DYNAMIC segment in image %p, returning", Base);
		return (void *)(hdr->e_entry + baseDiff);
	}

	dyntab = (void *)((Elf64_Addr)dyntab + baseDiff);

	// Parse the dynamic table (first pass)
	// - Search for String, Symbol and Hash tables
	for(i = 0; dyntab[i].d_tag != DT_NULL; i ++)
	{
		switch(dyntab[i].d_tag)
		{
		case DT_SYMTAB:
			dyntab[i].d_un.d_ptr += baseDiff;
			symtab = (void *)dyntab[i].d_un.d_ptr;
			break;
		case DT_STRTAB:
			dyntab[i].d_un.d_ptr += baseDiff;
			strtab = (void *)dyntab[i].d_un.d_ptr;
			break;
		case DT_HASH:
			dyntab[i].d_un.d_ptr += baseDiff;
			hashtab = (void *)dyntab[i].d_un.d_ptr;
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
			rel = (void *)dyntab[i].d_un.d_ptr;
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
			rela = (void *)dyntab[i].d_un.d_ptr;
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
			pltrel = (void *)dyntab[i].d_un.d_ptr;
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
	void _Elf64DoReloc(Elf64_Xword r_info, void *ptr, Elf64_Sxword addend)
	{
		 int	sym = ELF64_R_SYM(r_info);
		 int	type = ELF64_R_TYPE(r_info);
		const char	*symname = strtab + symtab[sym].st_name;
		switch( type )
		{
		case R_X86_64_NONE:
			break;
		case R_X86_64_64:
			*(uint64_t*)ptr = (uint64_t)GetSymbol(symname) + addend;
			break;
		case R_X86_64_COPY:
			break;
		case R_X86_64_GLOB_DAT:
			*(uint64_t*)ptr = (uint64_t)GetSymbol(symname);
			break;
		case R_X86_64_JUMP_SLOT:
			*(uint64_t*)ptr = (uint64_t)GetSymbol(symname);
			break;
		default:
			SysDebug("ld-acess - _Elf64DoReloc: Unknown relocation type %i", type);
			break;
		}
	}

	if( rel )
	{
		DEBUGS("rel_count = %i", rel_count);
		for( i = 0; i < rel_count; i ++ )
		{
			uint64_t *ptr = (void *)( rel[i].r_offset + baseDiff );
			_Elf64DoReloc( rel[i].r_info, ptr, *ptr);
		}
	}

	if( rela )
	{
		DEBUGS("rela_count = %i", rela_count);
		for( i = 0; i < rela_count; i ++ )
		{
			_Elf64DoReloc( rela[i].r_info, (void *)( rela[i].r_offset + baseDiff ), rela[i].r_addend );
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
				uint64_t *ptr = (void *)( plt[i].r_offset + baseDiff );
				_Elf64DoReloc( plt[i].r_info, ptr, *ptr);
			}
		}
		else {
			Elf64_Rela	*plt = pltrel;
			 int	count = plt_size / sizeof(Elf64_Rela);
			DEBUGS("plt rela count = %i", count);
			for( i = 0; i < count; i ++ )
			{
				_Elf64DoReloc( plt[i].r_info, (void *)(plt[i].r_offset + baseDiff), plt[i].r_addend);
			}
		}
	}

	DEBUGS("Elf64Relocate: Relocations done, return %p", (void *)(hdr->e_entry + baseDiff));
	return (void *)(hdr->e_entry + baseDiff);
}

void *Elf32Relocate(void *Base, char **envp, const char *Filename)
{
	Elf32_Ehdr	*hdr = Base;
	Elf32_Phdr	*phtab;
	 int	i, j;	// Counters
	char	*libPath;
	intptr_t	iRealBase = -1;
	intptr_t	iBaseDiff;
	 int	iSegmentCount;
	 int	iSymCount;
	Elf32_Rel	*rel = NULL;
	Elf32_Rela	*rela = NULL;
	Uint32	*pltgot = NULL;
	void	*plt = NULL;
	 int	relSz=0, relEntSz=8;
	 int	relaSz=0, relaEntSz=8;
	 int	pltSz=0, pltType=0;
	Elf32_Dyn	*dynamicTab = NULL;	// Dynamic Table Pointer
	char	*dynstrtab = NULL;	// .dynamic String Table
	Elf32_Sym	*dynsymtab;
	
	DEBUGS("ElfRelocate: (Base=0x%x)", Base);
	
	// Check magic header
	
	
	// Parse Program Header to get Dynamic Table
	phtab = Base + hdr->phoff;
	iSegmentCount = hdr->phentcount;
	for(i=0;i<iSegmentCount;i++)
	{
		// Determine linked base address
		if(phtab[i].Type == PT_LOAD && iRealBase > phtab[i].VAddr)
			iRealBase = phtab[i].VAddr;
		
		// Find Dynamic Section
		if(phtab[i].Type == PT_DYNAMIC) {
			if(dynamicTab) {
				DEBUGS(" WARNING - elf_relocate: Multiple PT_DYNAMIC segments");
				continue;
			}
			dynamicTab = (void *) (intptr_t) phtab[i].VAddr;
			j = i;	// Save Dynamic Table ID
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
	
	// Adjust Dynamic Table
	dynamicTab = (void *)( (intptr_t)dynamicTab + iBaseDiff );
	
	// === Get Symbol table and String Table ===
	for( j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
	{
		switch(dynamicTab[j].d_tag)
		{
		// --- Symbol Table ---
		case DT_SYMTAB:
			DEBUGS(" elf_relocate: DYNAMIC Symbol Table 0x%x (0x%x)",
				dynamicTab[j].d_val, dynamicTab[j].d_val + iBaseDiff);
			if(iBaseDiff != 0)	dynamicTab[j].d_val += iBaseDiff;
			dynsymtab = (void*)(dynamicTab[j].d_val);
//			hdr->misc.SymTable = dynamicTab[j].d_val;	// Saved in unused bytes of ident
			break;
		// --- String Table ---
		case DT_STRTAB:
			DEBUGS(" elf_relocate: DYNAMIC String Table 0x%x (0x%x)",
				dynamicTab[j].d_val, dynamicTab[j].d_val + iBaseDiff);
			if(iBaseDiff != 0)	dynamicTab[j].d_val += iBaseDiff;
			dynstrtab = (void*)(dynamicTab[j].d_val);
			break;
		// --- Hash Table --
		case DT_HASH:
			if(iBaseDiff != 0)	dynamicTab[j].d_val += iBaseDiff;
			iSymCount = ((Elf32_Word*)(dynamicTab[j].d_val))[1];
//			hdr->misc.HashTable = dynamicTab[j].d_val;	// Saved in unused bytes of ident
			break;
		}
	}

	if(dynsymtab == NULL) {
		SysDebug("ld-acess.so - WARNING: No Dynamic Symbol table in %p, returning", hdr);
		return (void *) hdr->entrypoint + iBaseDiff;
	}

	#if 0	
	// Alter Symbols to true base
	for(i=0;i<iSymCount;i++)
	{
		dynsymtab[i].value += iBaseDiff;
		dynsymtab[i].nameOfs += (intptr_t)dynstrtab;
		//DEBUGS("elf_relocate: Sym '%s' = 0x%x (relocated)", dynsymtab[i].name, dynsymtab[i].value);
	}
	#endif
	
	// === Add to loaded list (can be imported now) ===
	AddLoaded( Filename, Base );

	// === Parse Relocation Data ===
	DEBUGS(" elf_relocate: dynamicTab = 0x%x", dynamicTab);
	for( j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
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
			DEBUGS(" Required Library '%s'", libPath);
			if(LoadLibrary(libPath, NULL, envp) == 0) {
				#if DEBUG
				DEBUGS(" elf_relocate: Unable to load '%s'", libPath);
				#else
				SysDebug("Unable to load required library '%s'", libPath);
				#endif
				return 0;
			}
			break;
		// --- PLT/GOT ---
		case DT_PLTGOT:	pltgot = (void*)(iBaseDiff + dynamicTab[j].d_val);	break;
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
	
	void elf_doRelocate(uint32_t r_info, uint32_t *ptr, Elf32_Addr addend, Elf32_Sym *symtab)
	{
		 int	type = ELF32_R_TYPE(r_info);
		 int	sym = ELF32_R_SYM(r_info);
		Uint32	val;
		const char	*symname = dynstrtab + symtab[sym].nameOfs;
		switch( type )
		{
		// Standard 32 Bit Relocation (S+A)
		case R_386_32:
			val = (intptr_t) GetSymbol( symname );
			DEBUGS(" elf_doRelocate: R_386_32 *0x%x += 0x%x('%s')",
					ptr, val, symname);
			*ptr = val + addend;
			break;
			
		// 32 Bit Relocation wrt. Offset (S+A-P)
		case R_386_PC32:
			DEBUGS(" elf_doRelocate: #%i: '%s'", sym, symname);
			val = (intptr_t) GetSymbol( symname );
			DEBUGS(" elf_doRelocate: R_386_PC32 *0x%x = 0x%x + 0x%x - 0x%x",
				ptr, *ptr, val, (intptr_t)ptr );
			*ptr = val + addend - (intptr_t)ptr;
			//*ptr = val + addend - ((Uint)ptr - iBaseDiff);
			break;
	
		// Absolute Value of a symbol (S)
		case R_386_GLOB_DAT:
		case R_386_JMP_SLOT:
			DEBUGS(" elf_doRelocate: #%i: '%s'", sym, symname);
			val = (intptr_t) GetSymbol( symname );
			DEBUGS(" elf_doRelocate: %s *0x%x = 0x%x", csaR_NAMES[type], ptr, val);
			*ptr = val;
			break;
	
		// Base Address (B+A)
		case R_386_RELATIVE:
			DEBUGS(" elf_doRelocate: R_386_RELATIVE *0x%x = 0x%x + 0x%x", ptr, iBaseDiff, addend);
			*ptr = iBaseDiff + addend;
			break;
			
		default:
			DEBUGS(" elf_doRelocate: Rel 0x%x: 0x%x,%s", ptr, sym, csaR_NAMES[type]);
			break;
		}
	}
	
	// Parse Relocation Entries
	if(rel && relSz)
	{
		Uint32	*ptr;
		DEBUGS(" elf_relocate: rel=0x%x, relSz=0x%x, relEntSz=0x%x", rel, relSz, relEntSz);
		j = relSz / relEntSz;
		for( i = 0; i < j; i++ )
		{
			//DEBUGS("  Rel %i: 0x%x+0x%x", i, iBaseDiff, rel[i].r_offset);
			ptr = (void*)(iBaseDiff + rel[i].r_offset);
			elf_doRelocate(rel[i].r_info, ptr, *ptr, dynsymtab);
		}
	}
	// Parse Relocation Entries
	if(rela && relaSz)
	{
		Uint32	*ptr;
		DEBUGS(" elf_relocate: rela=0x%x, relaSz=0x%x, relaEntSz=0x%x", rela, relaSz, relaEntSz);
		j = relaSz / relaEntSz;
		for( i = 0; i < j; i++ )
		{
			ptr = (void*)(iBaseDiff + rela[i].r_offset);
			elf_doRelocate(rel[i].r_info, ptr, rela[i].r_addend, dynsymtab);
		}
	}
	
	// === Process PLT (Procedure Linkage Table) ===
	if(plt && pltSz)
	{
		Uint32	*ptr;
		DEBUGS(" elf_relocate: Relocate PLT, plt=0x%x", plt);
		if(pltType == DT_REL)
		{
			Elf32_Rel	*pltRel = plt;
			j = pltSz / sizeof(Elf32_Rel);
			DEBUGS(" elf_relocate: PLT Reloc Type = Rel, %i entries", j);
			for(i=0;i<j;i++)
			{
				ptr = (void*)(iBaseDiff + pltRel[i].r_offset);
				elf_doRelocate(pltRel[i].r_info, ptr, *ptr, dynsymtab);
			}
		}
		else
		{
			Elf32_Rela	*pltRela = plt;
			j = pltSz / sizeof(Elf32_Rela);
			DEBUGS(" elf_relocate: PLT Reloc Type = Rela, %i entries", j);
			for(i=0;i<j;i++)
			{
				ptr = (void*)(iRealBase + pltRela[i].r_offset);
				elf_doRelocate(pltRela[i].r_info, ptr, pltRela[i].r_addend, dynsymtab);
			}
		}
	}
	
	DEBUGS("ElfRelocate: RETURN 0x%x", hdr->entrypoint + iBaseDiff);
	return (void*)hdr->entrypoint + iBaseDiff;
}


/**
 * \fn int ElfGetSymbol(Uint Base, const char *name, void **ret)
 */
int ElfGetSymbol(void *Base, const char *Name, void **ret)
{
	Elf32_Ehdr	*hdr = Base;

	switch(hdr->e_ident[4])
	{
	case ELFCLASS32:
		return Elf32GetSymbol(Base, Name, ret);
	case ELFCLASS64:
		return Elf64GetSymbol(Base, Name, ret);
	default:
		SysDebug("ld-acess - ElfRelocate: Unknown file class %i", hdr->e_ident[4]);
		return 0;
	}
}

int Elf64GetSymbol(void *Base, const char *Name, void **Ret)
{
	Elf64_Ehdr	*hdr = Base;
	Elf64_Sym	*symtab;
	 int	nbuckets = 0;
	 int	iSymCount = 0;
	 int	i;
	Elf64_Word	*pBuckets;
	Elf64_Word	*pChains;
	uint32_t	iNameHash;
	const char	*dynstrtab;
	uintptr_t	iBaseDiff = -1;

//	DEBUGS("sizeof(uint32_t) = %i, sizeof(Elf64_Word) = %i", sizeof(uint32_t), sizeof(Elf64_Word));

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
		phtab = (void*)( Base + hdr->e_phoff );
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
//	DEBUGS("pBuckets = %p", pBuckets);

	nbuckets = pBuckets[0];
	iSymCount = pBuckets[1];
	pBuckets = &pBuckets[2];
//	DEBUGS("nbuckets = %i", nbuckets);
	pChains = &pBuckets[ nbuckets ];
	
	// Get hash
	iNameHash = ElfHashString(Name);
	iNameHash %= nbuckets;

	// Walk Chain
	i = pBuckets[ iNameHash ];
//	DEBUGS("dynstrtab = %p", dynstrtab);
//	DEBUGS("symtab = %p, i = %i", symtab, i);
	if(symtab[i].st_shndx != SHN_UNDEF && strcmp(dynstrtab + symtab[i].st_name, Name) == 0) {
		*Ret = (void*) (intptr_t) symtab[i].st_value + iBaseDiff;
		DEBUGS("%s = %p", Name, *Ret);
		return 1;
	}
	
	while(pChains[i] != STN_UNDEF)
	{
		i = pChains[i];
//		DEBUGS("chains i = %i", i);
		if(symtab[i].st_shndx != SHN_UNDEF && strcmp(dynstrtab + symtab[i].st_name, Name) == 0) {
			*Ret = (void*)(intptr_t)symtab[i].st_value + iBaseDiff;
			DEBUGS("%s = %p", Name, *Ret);
			return 1;
		}
	}
	
//	DEBUGS("Elf64GetSymbol: RETURN 0, Symbol '%s' not found", Name);
	return 0;
}

int Elf32GetSymbol(void *Base, const char *Name, void **ret)
{
	Elf32_Ehdr	*hdr = Base;
	Elf32_Sym	*symtab;
	 int	nbuckets = 0;
	 int	iSymCount = 0;
	 int	i;
	Uint32	*pBuckets;
	Uint32	*pChains;
	uint32_t	iNameHash;
	const char	*dynstrtab;
	uintptr_t	iBaseDiff = -1;

	//DEBUGS("ElfGetSymbol: (Base=0x%x, Name='%s')", Base, Name);
	dynstrtab = NULL;
	pBuckets = NULL;
	symtab = NULL;

	// Catch the current executable
	if( !pBuckets )
	{
		Elf32_Phdr	*phtab;
		Elf32_Dyn	*dynTab = NULL;
		 int	j;
		
		// Locate the tables
		phtab = (void*)( Base + hdr->phoff );
		for( i = 0; i < hdr->phentcount; i ++ )
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
		
		for( j = 0; dynTab[j].d_tag != DT_NULL; j++)
		{
			switch(dynTab[j].d_tag)
			{
			// --- Symbol Table ---
			case DT_SYMTAB:
				symtab = (void*)(intptr_t) dynTab[j].d_val;	// Rebased in Relocate
				break;
			case DT_STRTAB:
				dynstrtab = (void*)(intptr_t) dynTab[j].d_val;
				break;
			// --- Hash Table --
			case DT_HASH:
				pBuckets = (void*)(intptr_t) dynTab[j].d_val;
				break;
			}
		}
		
		#if 0
		hdr->misc.HashTable = pBuckets;
		hdr->misc.SymTable = symtab;
		hdr->misc.StrTab = dynstrtab;
		#endif
	}

	nbuckets = pBuckets[0];
	iSymCount = pBuckets[1];
	pBuckets = &pBuckets[2];
	pChains = &pBuckets[ nbuckets ];
	
	// Get hash
	iNameHash = ElfHashString(Name);
	iNameHash %= nbuckets;
	//DEBUGS(" ElfGetSymbol: iNameHash = 0x%x", iNameHash);

	// Walk Chain
	i = pBuckets[ iNameHash ];
	//DEBUGS(" ElfGetSymbol: strcmp(Name, \"%s\")", symtab[i].name);
	if(symtab[i].shndx != SHN_UNDEF && strcmp(dynstrtab + symtab[i].nameOfs, Name) == 0) {
		*ret = (void*) (intptr_t) symtab[ i ].value + iBaseDiff;
		return 1;
	}
	
	//DEBUGS(" ElfGetSymbol: Hash of first = 0x%x", ElfHashString( symtab[i].name ) % nbuckets);
	while(pChains[i] != STN_UNDEF)
	{
		//DEBUGS(" pChains[%i] = %i", i, pChains[i]);
		i = pChains[i];
		//DEBUGS(" ElfGetSymbol: strcmp(Name, \"%s\")", symtab[ i ].name);
		if(symtab[i].shndx != SHN_UNDEF && strcmp(dynstrtab + symtab[ i ].nameOfs, Name) == 0) {
			//DEBUGS("ElfGetSymbol: RETURN 1, '%s' = 0x%x", symtab[ i ].name, symtab[ i ].value);
			*ret = (void*)(intptr_t)symtab[ i ].value + iBaseDiff;
			return 1;
		}
	}
	
	//DEBUGS("ElfGetSymbol: RETURN 0, Symbol '%s' not found", Name);
	return 0;
}

Uint32 ElfHashString(const char *name)
{
	Uint32	h = 0, g;
	while(*name)
	{
		h = (h << 4) + *(Uint8*)name++;
		if( (g = h & 0xf0000000) )
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

#if 0
unsigned long elf_hash(const unsigned char *name)
{
	unsigned long	h = 0, g;
	while (*name)
	{
		h = (h << 4) + *name++;
		if (g = h & 0xf0000000)
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}
#endif
