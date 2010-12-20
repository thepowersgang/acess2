/*
 * Acess v0.1
 * ELF Executable Loader Code
 */
#define DEBUG	0
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "elf.h"

#define DEBUG_WARN	1

#define ENTER(...)
#define LOG(...)
#define LEAVE(...)

// === PROTOTYPES ===
 int	Elf_Load(int fd);
 int	Elf_Relocate(void *Base);
 int	Elf_GetSymbol(void *Base, char *Name, intptr_t *ret);
 int	Elf_Int_DoRelocate(uint32_t r_info, uint32_t *ptr, uint32_t addend, Elf32_Sym *symtab, intptr_t base);
uint32_t	Elf_Int_HashString(char *str);

// === CODE ===
int Elf_Load(int FD)
{
	Elf32_Ehdr	hdr;
	Elf32_Phdr	*phtab;
	 int	i, j;
	 int	iPageCount;
	uint32_t	max, base = -1;
	uint32_t	addr;
	
	ENTER("xFD", FD);
	
	// Read ELF Header
	read(FD, &hdr, sizeof(hdr));
	
	// Check the file type
	if(hdr.ident[0] != 0x7F || hdr.ident[1] != 'E' || hdr.ident[2] != 'L' || hdr.ident[3] != 'F') {
		Warning("Non-ELF File was passed to the ELF loader\n");
		LEAVE('n');
		return 1;
	}
	
	// Check for a program header
	if(hdr.phoff == 0) {
		#if DEBUG_WARN
		Warning("ELF File does not contain a program header\n");
		#endif
		LEAVE('n');
		return 1;
	}
	
	// Read Program Header Table
	phtab = malloc( sizeof(Elf32_Phdr) * hdr.phentcount );
	if( !phtab ) {
		LEAVE('n');
		return 1;
	}
	LOG("hdr.phoff = 0x%08x", hdr.phoff);
	lseek(FD, hdr.phoff, SEEK_SET);
	read(FD, phtab, sizeof(Elf32_Phdr)*hdr.phentcount);
	
	// Count Pages
	iPageCount = 0;
	LOG("hdr.phentcount = %i", hdr.phentcount);
	for( i = 0; i < hdr.phentcount; i++ )
	{
		// Ignore Non-LOAD types
		if(phtab[i].Type != PT_LOAD)
			continue;
		iPageCount += ((phtab[i].VAddr&0xFFF) + phtab[i].MemSize + 0xFFF) >> 12;
		LOG("phtab[%i] = {VAddr:0x%x, MemSize:0x%x}", i, phtab[i].VAddr, phtab[i].MemSize);
	}
	
	LOG("iPageCount = %i", iPageCount);
	
	// Allocate Information Structure
	//ret = malloc( sizeof(tBinary) + sizeof(tBinaryPage)*iPageCount );
	// Fill Info Struct
	//ret->Entry = hdr.entrypoint;
	//ret->Base = -1;		// Set Base to maximum value
	//ret->NumPages = iPageCount;
	//ret->Interpreter = NULL;

	// Prescan for base and size
	for( i = 0; i < hdr.phentcount; i ++)
	{
		if( phtab[i].Type != PT_LOAD )
			continue;
		if( phtab[i].VAddr < base )
			base = phtab[i].VAddr;
		if( phtab[i].VAddr > max )
			max = phtab[i].VAddr;
	}

	LOG("base = %08x, max = %08x", base, max);
	
	// Load Pages
	j = 0;
	for( i = 0; i < hdr.phentcount; i++ )
	{
		//LOG("phtab[%i].Type = 0x%x", i, phtab[i].Type);
		LOG("phtab[%i] = {", i);
		LOG(" .Type = 0x%08x", phtab[i].Type);
		LOG(" .Offset = 0x%08x", phtab[i].Offset);
		LOG(" .VAddr = 0x%08x", phtab[i].VAddr);
		LOG(" .PAddr = 0x%08x", phtab[i].PAddr);
		LOG(" .FileSize = 0x%08x", phtab[i].FileSize);
		LOG(" .MemSize = 0x%08x", phtab[i].MemSize);
		LOG(" .Flags = 0x%08x", phtab[i].Flags);
		LOG(" .Align = 0x%08x", phtab[i].Align);
		LOG(" }");
		// Get Interpreter Name
		if( phtab[i].Type == PT_INTERP )
		{
			char *tmp;
			//if(ret->Interpreter)	continue;
			tmp = malloc(phtab[i].FileSize);
			lseek(FD, phtab[i].Offset, 1);
			read(FD, tmp, phtab[i].FileSize);
			//ret->Interpreter = Binary_RegInterp(tmp);
			LOG("Interpreter '%s'", tmp);
			free(tmp);
			continue;
		}
		// Ignore non-LOAD types
		if(phtab[i].Type != PT_LOAD)	continue;
		
		LOG("phtab[%i] = {VAddr:0x%x,Offset:0x%x,FileSize:0x%x}",
			i, phtab[i].VAddr, phtab[i].Offset, phtab[i].FileSize);
		
		addr = phtab[i].VAddr;

		AllocateMemory( addr, phtab[i].MemSize );
		
		lseek(FD, phtab[i].Offset, SEEK_SET);
		read(FD, (void*)(intptr_t)addr, phtab[i].FileSize);
		memset( (char*)(intptr_t)addr + phtab[i].FileSize, 0, phtab[i].MemSize - phtab[i].FileSize);
	}
	
	// Clean Up
	free(phtab);
	// Return
	LEAVE('i', 0);
	return 0;
}

// --- ELF RELOCATION ---
/**
 \fn int Elf_Relocate(void *Base)
 \brief Relocates a loaded ELF Executable
*/
int Elf_Relocate(void *Base)
{
	Elf32_Ehdr	*hdr = Base;
	Elf32_Phdr	*phtab;
	 int	i, j;	// Counters
	char	*libPath;
	uint32_t	iRealBase = -1;
	intptr_t	iBaseDiff;
	 int	iSegmentCount;
	 int	iSymCount = 0;
	Elf32_Rel	*rel = NULL;
	Elf32_Rela	*rela = NULL;
	uint32_t	*pltgot = NULL;
	void	*plt = NULL;
	uint32_t	*ptr;
	 int	relSz=0, relEntSz=8;
	 int	relaSz=0, relaEntSz=8;
	 int	pltSz=0, pltType=0;
	Elf32_Dyn	*dynamicTab = NULL;	// Dynamic Table Pointer
	char	*dynstrtab = NULL;	// .dynamic String Table
	Elf32_Sym	*dynsymtab = NULL;
	 int	bFailed = 0;
	
	ENTER("pBase", Base);
	
	// Parse Program Header to get Dynamic Table
	phtab = Base + hdr->phoff;
	iSegmentCount = hdr->phentcount;
	for(i = 0; i < iSegmentCount; i ++ )
	{
		// Determine linked base address
		if(phtab[i].Type == PT_LOAD && iRealBase > phtab[i].VAddr)
			iRealBase = phtab[i].VAddr;
		
		// Find Dynamic Section
		if(phtab[i].Type == PT_DYNAMIC) {
			if(dynamicTab) {
				Warning("Elf_Relocate - Multiple PT_DYNAMIC segments\n");
				continue;
			}
			dynamicTab = (void *) (intptr_t) phtab[i].VAddr;
			j = i;	// Save Dynamic Table ID
			break;
		}
	}
	
	// Check if a PT_DYNAMIC segement was found
	if(!dynamicTab) {
		Warning("Elf_Relocate: No PT_DYNAMIC segment in image, returning\n");
		LEAVE('x', hdr->entrypoint);
		return hdr->entrypoint;
	}
	
	// Page Align real base
	iRealBase &= ~0xFFF;
	
	// Adjust "Real" Base
	iBaseDiff = (intptr_t)Base - iRealBase;
	// Adjust Dynamic Table
	dynamicTab = (void *) ((intptr_t)dynamicTab + iBaseDiff);
	
	// === Get Symbol table and String Table ===
	for( j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
	{
		switch(dynamicTab[j].d_tag)
		{
		// --- Symbol Table ---
		case DT_SYMTAB:
			dynamicTab[j].d_val += iBaseDiff;
			dynsymtab = (void*) (intptr_t) dynamicTab[j].d_val;
			hdr->misc.SymTable = dynamicTab[j].d_val;	// Saved in unused bytes of ident
			break;
		
		// --- String Table ---
		case DT_STRTAB:
			dynamicTab[j].d_val += iBaseDiff;
			dynstrtab = (void*) (intptr_t) dynamicTab[j].d_val;
			break;
		
		// --- Hash Table --
		case DT_HASH:
			dynamicTab[j].d_val += iBaseDiff;
			iSymCount = ((uint32_t*)((intptr_t)dynamicTab[j].d_val))[1];
			hdr->misc.HashTable = dynamicTab[j].d_val;	// Saved in unused bytes of ident
			break;
		}
	}


	// Alter Symbols to true base
	for(i = 0; i < iSymCount; i ++)
	{
		dynsymtab[i].value += iBaseDiff;
		dynsymtab[i].nameOfs += (intptr_t)dynstrtab;
		//LOG("Sym '%s' = 0x%x (relocated)\n", dynsymtab[i].name, dynsymtab[i].value);
	}
	
	// === Add to loaded list (can be imported now) ===
	//Binary_AddLoaded( (intptr_t)Base );

	// === Parse Relocation Data ===
	for( j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
	{
		switch(dynamicTab[j].d_tag)
		{
		// --- Shared Library Name ---
		case DT_SONAME:
			LOG(".so Name '%s'\n", dynstrtab+dynamicTab[j].d_val);
			break;
		// --- Needed Library ---
		case DT_NEEDED:
			libPath = dynstrtab + dynamicTab[j].d_val;
			Notice("%p - Required Library '%s' - TODO load DT_NEEDED\n", Base, libPath);
			break;
		// --- PLT/GOT ---
		case DT_PLTGOT:	pltgot = (void*)(iBaseDiff+dynamicTab[j].d_val);	break;
		case DT_JMPREL:	plt = (void*)(iBaseDiff+dynamicTab[j].d_val);	break;
		case DT_PLTREL:	pltType = dynamicTab[j].d_val;	break;
		case DT_PLTRELSZ:	pltSz = dynamicTab[j].d_val;	break;
		
		// --- Relocation ---
		case DT_REL:	rel = (void*)(iBaseDiff + dynamicTab[j].d_val);	break;
		case DT_RELSZ:	relSz = dynamicTab[j].d_val;	break;
		case DT_RELENT:	relEntSz = dynamicTab[j].d_val;	break;
		
		case DT_RELA:	rela = (void*)(iBaseDiff + dynamicTab[j].d_val);	break;
		case DT_RELASZ:	relaSz = dynamicTab[j].d_val;	break;
		case DT_RELAENT:	relaEntSz = dynamicTab[j].d_val;	break;
		}
	}
	
	// Parse Relocation Entries
	if(rel && relSz)
	{
		j = relSz / relEntSz;
		for( i = 0; i < j; i++ )
		{
			ptr = (void*)(iBaseDiff + rel[i].r_offset);
			if( !Elf_Int_DoRelocate(rel[i].r_info, ptr, *ptr, dynsymtab, (intptr_t)Base) ) {
				bFailed = 1;
			}
		}
	}
	// Parse Relocation Entries
	if(rela && relaSz)
	{
		j = relaSz / relaEntSz;
		for( i = 0; i < j; i++ )
		{
			ptr = (void*)(iBaseDiff + rela[i].r_offset);
			if( !Elf_Int_DoRelocate(rel[i].r_info, ptr, rela[i].r_addend, dynsymtab, (intptr_t)Base) ) {
				bFailed = 1;
			}
		}
	}
	
	// === Process PLT (Procedure Linkage Table) ===
	if(plt && pltSz)
	{
		if(pltType == DT_REL)
		{
			Elf32_Rel	*pltRel = plt;
			j = pltSz / sizeof(Elf32_Rel);
			LOG("PLT Rel - plt = %p, pltSz = %i (%i ents)", plt, pltSz, j);
			for(i = 0; i < j; i++)
			{
				ptr = (void*)(iBaseDiff + pltRel[i].r_offset);
				if( !Elf_Int_DoRelocate(pltRel[i].r_info, ptr, *ptr, dynsymtab, (intptr_t)Base) ) {
					bFailed = 1;
				}
			}
		}
		else
		{
			Elf32_Rela	*pltRela = plt;
			j = pltSz / sizeof(Elf32_Rela);
			LOG("PLT RelA - plt = %p, pltSz = %i (%i ents)", plt, pltSz, j);
			for(i=0;i<j;i++)
			{
				ptr = (void*)(iBaseDiff + pltRela[i].r_offset);
				if( !Elf_Int_DoRelocate(pltRela[i].r_info, ptr, pltRela[i].r_addend, dynsymtab, (intptr_t)Base) ) {
					bFailed = 1;
				}
			}
		}
	}
	
	if(bFailed) {
		LEAVE('i', 0);
		return 0;
	}
	
	LEAVE('x', hdr->entrypoint);
	return hdr->entrypoint;
}

/**
 * \fn void Elf_Int_DoRelocate(uint32_t r_info, uint32_t *ptr, uint32_t addend, Elf32_Sym *symtab, uint32_t base)
 * \brief Performs a relocation
 * \param r_info	Field from relocation entry
 * \param ptr	Pointer to location of relocation
 * \param addend	Value to add to symbol
 * \param symtab	Symbol Table
 * \param base	Base of loaded binary
 */
int Elf_Int_DoRelocate(uint32_t r_info, uint32_t *ptr, uint32_t addend, Elf32_Sym *symtab, intptr_t base)
{
	intptr_t	val;
	 int	type = ELF32_R_TYPE(r_info);
	 int	sym = ELF32_R_SYM(r_info);
	char	*sSymName = symtab[sym].name;
	
	//LogF("Elf_Int_DoRelocate: (r_info=0x%x, ptr=0x%x, addend=0x%x, .., base=0x%x)\n",
	//	r_info, ptr, addend, base);
	
	switch( type )
	{
	// Standard 32 Bit Relocation (S+A)
	case R_386_32:
		if( !Elf_GetSymbol((void*)base, sSymName, &val) )	// Search this binary first
			if( !Binary_GetSymbol( sSymName, &val ) )
				return 0;
		LOG("%08x R_386_32 *0x%x += 0x%x('%s')", r_info, ptr, val, sSymName);
		*ptr = val + addend;
		break;
		
	// 32 Bit Relocation wrt. Offset (S+A-P)
	case R_386_PC32:
		if( !Elf_GetSymbol( (void*)base, sSymName, &val ) )
			if( !Binary_GetSymbol( sSymName, &val ) )
				return 0;
		LOG("%08x R_386_PC32 *0x%x = 0x%x + 0x%x('%s') - %p", r_info, ptr, *ptr, val, sSymName, ptr );
		// TODO: Check if it needs the true value of ptr or the compiled value
		// NOTE: Testing using true value
		*ptr = val + addend - (intptr_t)ptr;
		break;

	// Absolute Value of a symbol (S)
	case R_386_GLOB_DAT:
		if( !Elf_GetSymbol( (void*)base, sSymName, &val ) )
			if( !Binary_GetSymbol( sSymName, &val ) )
				return 0;
		LOG("%08x R_386_GLOB_DAT *0x%x = 0x%x (%s)", r_info, ptr, val, sSymName);
		*ptr = val;
		break;
	
	// Absolute Value of a symbol (S)
	case R_386_JMP_SLOT:
		if( !Elf_GetSymbol( (void*)base, sSymName, &val ) )
			if( !Binary_GetSymbol( sSymName, &val ) )
				return 0;
		LOG("%08x R_386_JMP_SLOT %p = 0x%x (%s)", r_info, ptr, val, sSymName);
		*ptr = val;
		break;

	// Base Address (B+A)
	case R_386_RELATIVE:
		LOG("%08x R_386_RELATIVE %p = 0x%x + 0x%x", r_info, ptr, base, addend);
		*ptr = base + addend;
		break;
		
	default:
		LOG("Rel 0x%x: 0x%x,%i", ptr, sym, type);
		break;
	}
	return 1;
}

/**
 * \fn int Elf_GetSymbol(void *Base, char *name, intptr_t *ret)
 * \brief Get a symbol from the loaded binary
 */
int Elf_GetSymbol(void *Base, char *Name, intptr_t *ret)
{
	Elf32_Ehdr	*hdr = (void*)Base;
	Elf32_Sym	*symtab;
	 int	nbuckets = 0;
	 int	iSymCount = 0;
	 int	i;
	uint32_t	*pBuckets;
	uint32_t	*pChains;
	uint32_t	iNameHash;

	if(!Base)	return 0;

	pBuckets = (void *)(intptr_t) hdr->misc.HashTable;
	symtab = (void *)(intptr_t) hdr->misc.SymTable;
	
	nbuckets = pBuckets[0];
	iSymCount = pBuckets[1];
	pBuckets = &pBuckets[2];
	pChains = &pBuckets[ nbuckets ];
	
	// Get hash
	iNameHash = Elf_Int_HashString(Name);
	iNameHash %= nbuckets;

	// Check Bucket
	i = pBuckets[ iNameHash ];
	if(symtab[i].shndx != SHN_UNDEF && strcmp(symtab[i].name, Name) == 0) {
		if(ret)	*ret = symtab[ i ].value;
		return 1;
	}
	
	// Walk Chain
	while(pChains[i] != STN_UNDEF)
	{
		i = pChains[i];
		if(symtab[i].shndx != SHN_UNDEF && strcmp(symtab[ i ].name, Name) == 0) {
			if(ret)	*ret = symtab[ i ].value;
			return 1;
		}
	}
	return 0;
}

/**
 * \fn uint32_t Elf_Int_HashString(char *str)
 * \brief Hash a string in the ELF format
 * \param str	String to hash
 * \return Hash value
 */
uint32_t Elf_Int_HashString(char *str)
{
	uint32_t	h = 0, g;
	while(*str)
	{
		h = (h << 4) + *str++;
		if( (g = h & 0xf0000000) )
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}
