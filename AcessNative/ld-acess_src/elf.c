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

#define MKPTR(_type,_val)	((_type*)(uintptr_t)(_val))
#define PTRMK(_type,_val)	MKPTR(_type,_val)
#define PTR(_val)	((void*)(uintptr_t)(_val))

#if DEBUG
# define ENTER(...)
# define LOG(s, ...)	printf("%s: " s, __func__, __VA_ARGS__)
# define LOGS(s)	printf("%s: " s, __func__)
# define LEAVE(...)
#else
# define ENTER(...)
# define LOG(...)
# define LOGS(...)
# define LEAVE(...)
#endif

// === PROTOTYPES ===
void	*Elf_Load(int FD);
uintptr_t	Elf_Relocate(void *Base);
 int	Elf_GetSymbol(void *Base, char *Name, uintptr_t *ret);
 int	Elf_Int_DoRelocate(uint32_t r_info, uint32_t *ptr, uint32_t addend, Elf32_Sym *symtab, void *Base);
uint32_t	Elf_Int_HashString(char *str);

// === CODE ===
void *Elf_Load(int FD)
{
	Elf32_Ehdr	hdr;
	Elf32_Phdr	*phtab;
	 int	i, j;
	 int	iPageCount;
	uint32_t	max, base;
	uint32_t	addr;
	uint32_t	baseDiff = 0;
	
	ENTER("iFD", FD);
	
	// Read ELF Header
	acess_read(FD, sizeof(hdr), &hdr);
	
	// Check the file type
	if(hdr.ident[0] != 0x7F || hdr.ident[1] != 'E' || hdr.ident[2] != 'L' || hdr.ident[3] != 'F') {
		Warning("Non-ELF File was passed to the ELF loader\n");
		LEAVE('n');
		return NULL;
	}
	
	// Check for a program header
	if(hdr.phoff == 0) {
		#if DEBUG_WARN
		Warning("ELF File does not contain a program header\n");
		#endif
		LEAVE('n');
		return NULL;
	}
	
	// Read Program Header Table
	phtab = malloc( sizeof(Elf32_Phdr) * hdr.phentcount );
	if( !phtab ) {
		LEAVE('n');
		return NULL;
	}
	LOG("hdr.phoff = 0x%08x\n", hdr.phoff);
	acess_seek(FD, hdr.phoff, ACESS_SEEK_SET);
	acess_read(FD, sizeof(Elf32_Phdr) * hdr.phentcount, phtab);
	
	// Count Pages
	iPageCount = 0;
	LOG("hdr.phentcount = %i\n", hdr.phentcount);
	for( i = 0; i < hdr.phentcount; i++ )
	{
		// Ignore Non-LOAD types
		if(phtab[i].Type != PT_LOAD)
			continue;
		iPageCount += ((phtab[i].VAddr&0xFFF) + phtab[i].MemSize + 0xFFF) >> 12;
		LOG("phtab[%i] = {VAddr:0x%x, MemSize:0x%x}\n", i, phtab[i].VAddr, phtab[i].MemSize);
	}
	
	LOG("iPageCount = %i\n", iPageCount);
	
	// Allocate Information Structure
	//ret = malloc( sizeof(tBinary) + sizeof(tBinaryPage)*iPageCount );
	// Fill Info Struct
	//ret->Entry = hdr.entrypoint;
	//ret->Base = -1;		// Set Base to maximum value
	//ret->NumPages = iPageCount;
	//ret->Interpreter = NULL;

	// Prescan for base and size
	max = 0;
	base = 0xFFFFFFFF;
	for( i = 0; i < hdr.phentcount; i ++)
	{
		if( phtab[i].Type != PT_LOAD )
			continue;
		if( phtab[i].VAddr < base )
			base = phtab[i].VAddr;
		if( phtab[i].VAddr + phtab[i].MemSize > max )
			max = phtab[i].VAddr + phtab[i].MemSize;
	}

	LOG("base = %08x, max = %08x\n", base, max);

	if( base == 0 ) {
		// Find a nice space (31 address bits allowed)
		base = FindFreeRange( max, 31 );
		LOG("new base = %08x\n", base);
		if( base == 0 )	return NULL;
		baseDiff = base;
	}
	
	// Load Pages
	j = 0;
	for( i = 0; i < hdr.phentcount; i++ )
	{
		//LOG("phtab[%i].Type = 0x%x", i, phtab[i].Type);
		LOG("phtab[%i] = {\n", i);
		LOG(" .Type = 0x%08x\n", phtab[i].Type);
		LOG(" .Offset = 0x%08x\n", phtab[i].Offset);
		LOG(" .VAddr = 0x%08x\n", phtab[i].VAddr);
		LOG(" .PAddr = 0x%08x\n", phtab[i].PAddr);
		LOG(" .FileSize = 0x%08x\n", phtab[i].FileSize);
		LOG(" .MemSize = 0x%08x\n", phtab[i].MemSize);
		LOG(" .Flags = 0x%08x\n", phtab[i].Flags);
		LOG(" .Align = 0x%08x\n", phtab[i].Align);
		LOGS(" }\n");
		// Get Interpreter Name
		if( phtab[i].Type == PT_INTERP )
		{
			char *tmp;
			//if(ret->Interpreter)	continue;
			tmp = malloc(phtab[i].FileSize);
			acess_seek(FD, phtab[i].Offset, ACESS_SEEK_SET);
			acess_read(FD, phtab[i].FileSize, tmp);
			//ret->Interpreter = Binary_RegInterp(tmp);
			LOG("Interpreter '%s'\n", tmp);
			free(tmp);
			continue;
		}
		// Ignore non-LOAD types
		if(phtab[i].Type != PT_LOAD)	continue;
		
		LOG("phtab[%i] = {VAddr:0x%x,Offset:0x%x,FileSize:0x%x}\n",
			i, phtab[i].VAddr+baseDiff, phtab[i].Offset, phtab[i].FileSize);
		
		addr = phtab[i].VAddr + baseDiff;

		if( AllocateMemory( addr, phtab[i].MemSize ) ) {
			fprintf(stderr, "Elf_Load: Unable to map memory at %x (0x%x bytes)\n",
				addr, phtab[i].MemSize);
			free( phtab );
			return NULL;
		}
		
		acess_seek(FD, phtab[i].Offset, ACESS_SEEK_SET);
		acess_read(FD, phtab[i].FileSize, PTRMK(void, addr) );
		memset( PTRMK(char, addr) + phtab[i].FileSize, 0, phtab[i].MemSize - phtab[i].FileSize );
	}
	
	// Clean Up
	free(phtab);
	// Return
	LEAVE('p', base);
	return PTRMK(void, base);
}

// --- ELF RELOCATION ---
/**
 * \brief Relocates a loaded ELF Executable
 */
uintptr_t Elf_Relocate(void *Base)
{
	Elf32_Ehdr	*hdr = Base;
	Elf32_Phdr	*phtab;
	 int	i, j;	// Counters
	char	*libPath;
	uint32_t	iRealBase = -1;
	uintptr_t	iBaseDiff;
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
	LOG("Base = %p\n", Base);
	
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
			dynamicTab = MKPTR(void, phtab[i].VAddr);
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
	
	LOG("dynamicTab = %p\n", dynamicTab);
	// Adjust "Real" Base
	iBaseDiff = (uintptr_t)Base - iRealBase;
	LOG("iBaseDiff = %p\n", (void*)iBaseDiff);
	// Adjust Dynamic Table
	dynamicTab = PTR( (uintptr_t)dynamicTab + iBaseDiff);
	LOG("dynamicTab = %p\n", dynamicTab);

	hdr->entrypoint += iBaseDiff;
	
	hdr->misc.SymTable = 0;
	hdr->misc.HashTable = 0;
	
	// === Get Symbol table and String Table ===
	for( j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
	{
		switch(dynamicTab[j].d_tag)
		{
		// --- Symbol Table ---
		case DT_SYMTAB:
			dynamicTab[j].d_val += iBaseDiff;
			dynsymtab = PTRMK(void, dynamicTab[j].d_val);
			hdr->misc.SymTable = dynamicTab[j].d_val;	// Saved in unused bytes of ident
			break;
		
		// --- String Table ---
		case DT_STRTAB:
			dynamicTab[j].d_val += iBaseDiff;
			dynstrtab = PTRMK(void, dynamicTab[j].d_val);
			break;
		
		// --- Hash Table --
		case DT_HASH:
			dynamicTab[j].d_val += iBaseDiff;
			iSymCount = (PTRMK(uint32_t, dynamicTab[j].d_val))[1];
			hdr->misc.HashTable = dynamicTab[j].d_val;	// Saved in unused bytes of ident
			break;
		}
	}
	
	LOG("hdr->misc.SymTable = %x, hdr->misc.HashTable = %x",
		hdr->misc.SymTable, hdr->misc.HashTable);


	// Alter Symbols to true base
	for(i = 0; i < iSymCount; i ++)
	{
		dynsymtab[i].nameOfs += (uintptr_t)dynstrtab;
		if( dynsymtab[i].shndx == SHN_UNDEF )
		{
			LOG("Sym '%s' = UNDEF\n", MKPTR(char,dynsymtab[i].name));
		}
		else
		{
			dynsymtab[i].value += iBaseDiff;
			LOG("Sym '%s' = 0x%x (relocated)\n", MKPTR(char,dynsymtab[i].name), dynsymtab[i].value);
		}
	}
	
	// === Add to loaded list (can be imported now) ===
	Binary_SetReadyToUse( Base );

	// === Parse Relocation Data ===
	for( j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
	{
		switch(dynamicTab[j].d_tag)
		{
		// --- Shared Library Name ---
		case DT_SONAME:
			LOG(".so Name '%s'\n", dynstrtab + dynamicTab[j].d_val);
			break;
		// --- Needed Library ---
		case DT_NEEDED:
			libPath = dynstrtab + dynamicTab[j].d_val;
			Binary_LoadLibrary(libPath);
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
			if( !Elf_Int_DoRelocate(rel[i].r_info, ptr, *ptr, dynsymtab, Base) ) {
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
			if( !Elf_Int_DoRelocate(rel[i].r_info, ptr, rela[i].r_addend, dynsymtab, Base) ) {
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
			LOG("PLT Rel - plt = %p, pltSz = %i (%i ents)\n", plt, pltSz, j);
			for(i = 0; i < j; i++)
			{
				ptr = (void*)(iBaseDiff + pltRel[i].r_offset);
				if( !Elf_Int_DoRelocate(pltRel[i].r_info, ptr, *ptr, dynsymtab, Base) ) {
					bFailed = 1;
				}
			}
		}
		else
		{
			Elf32_Rela	*pltRela = plt;
			j = pltSz / sizeof(Elf32_Rela);
			LOG("PLT RelA - plt = %p, pltSz = %i (%i ents)\n", plt, pltSz, j);
			for(i=0;i<j;i++)
			{
				ptr = (void*)(iBaseDiff + pltRela[i].r_offset);
				if( !Elf_Int_DoRelocate(pltRela[i].r_info, ptr, pltRela[i].r_addend, dynsymtab, Base) ) {
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
 * \fn void Elf_Int_DoRelocate(uint32_t r_info, uint32_t *ptr, uint32_t addend, Elf32_Sym *symtab, void *base)
 * \brief Performs a relocation
 * \param r_info	Field from relocation entry
 * \param ptr	Pointer to location of relocation
 * \param addend	Value to add to symbol
 * \param symtab	Symbol Table
 * \param base	Base of loaded binary
 */
int Elf_Int_DoRelocate(uint32_t r_info, uint32_t *ptr, uint32_t addend, Elf32_Sym *symtab, void *base)
{
	uintptr_t	val;
	 int	type = ELF32_R_TYPE(r_info);
	 int	sym = ELF32_R_SYM(r_info);
	char	*sSymName = PTRMK(char, symtab[sym].name);
	
	//LogF("Elf_Int_DoRelocate: (r_info=0x%x, ptr=0x%x, addend=0x%x, .., base=0x%x)\n",
	//	r_info, ptr, addend, base);
	
	switch( type )
	{
	// Standard 32 Bit Relocation (S+A)
	case R_386_32:
		if( !Elf_GetSymbol( base, sSymName, &val ) && !Binary_GetSymbol( sSymName, &val ) ) {
			Warning("Unable to find symbol '%s'", sSymName);
			return 0;
		}
		LOG("%08x R_386_32 *%p += %p('%s')\n", r_info, ptr, (void*)val, sSymName);
		*ptr = val + addend;
		break;
		
	// 32 Bit Relocation wrt. Offset (S+A-P)
	case R_386_PC32:
		if( !Elf_GetSymbol( base, sSymName, &val ) && !Binary_GetSymbol( sSymName, &val ) ) {
			Warning("Unable to find symbol '%s'", sSymName);
			return 0;
		}
		LOG("%08x R_386_PC32 *%p = 0x%x + %p('%s') - %p\n", r_info, ptr, *ptr, (void*)val, sSymName, ptr );
		// TODO: Check if it needs the true value of ptr or the compiled value
		// NOTE: Testing using true value
		*ptr = val + addend - (uintptr_t)ptr;
		break;

	// Absolute Value of a symbol (S)
	case R_386_GLOB_DAT:
		if( !Elf_GetSymbol( base, sSymName, &val ) && !Binary_GetSymbol( sSymName, &val ) ) {
			Warning("Unable to find symbol '%s'", sSymName);
			return 0; 
		}
		LOG("%08x R_386_GLOB_DAT *%p = 0x%x(%s)\n", r_info, ptr, (unsigned int)val, sSymName);
		*ptr = val;
		break;
	
	// Absolute Value of a symbol (S)
	case R_386_JMP_SLOT:
		if( !Elf_GetSymbol( base, sSymName, &val ) && !Binary_GetSymbol( sSymName, &val ) ) {
			Warning("Unable to find symbol '%s'", sSymName);
			return 0;
		}
		LOG("%08x R_386_JMP_SLOT *%p = 0x%x (%s)\n", r_info, ptr, (unsigned int)val, sSymName);
		*ptr = val;
		break;

	// Base Address (B+A)
	case R_386_RELATIVE:
		LOG("%08x R_386_RELATIVE *%p = %p + 0x%x\n", r_info, ptr, base, addend);
		*ptr = (uintptr_t)base + addend;
		break;
		
	default:
		LOG("Rel %p: 0x%x,%i\n", ptr, sym, type);
		break;
	}
	return 1;
}

/**
 * \fn int Elf_GetSymbol(void *Base, char *name, uintptr_t *ret)
 * \brief Get a symbol from the loaded binary
 */
int Elf_GetSymbol(void *Base, char *Name, uintptr_t *ret)
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

	pBuckets = PTR(hdr->misc.HashTable);
	symtab = PTR(hdr->misc.SymTable);
	
//	LOG("Base = %p : pBuckets = %p, symtab = %p\n", Base, pBuckets, symtab);
	
	if(!pBuckets || !symtab)
		return 0;
	
	nbuckets = pBuckets[0];
	iSymCount = pBuckets[1];
	pBuckets = &pBuckets[2];
	pChains = &pBuckets[ nbuckets ];
	
	// Get hash
	iNameHash = Elf_Int_HashString(Name);
	iNameHash %= nbuckets;

	// Check Bucket
	i = pBuckets[ iNameHash ];
	if(symtab[i].shndx != SHN_UNDEF && strcmp(MKPTR(char,symtab[i].name), Name) == 0) {
		if(ret)	*ret = symtab[ i ].value;
		return 1;
	}
	
	// Walk Chain
	while(pChains[i] != STN_UNDEF)
	{
		i = pChains[i];
		if(symtab[i].shndx != SHN_UNDEF && strcmp(MKPTR(char,symtab[i].name), Name) == 0) {
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
