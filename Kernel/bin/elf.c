/*
 * Acess v0.1
 * ELF Executable Loader Code
 */
#define DEBUG	0
#include <acess.h>
#include <binary.h>
#include "elf.h"

#define DEBUG_WARN	1

// === PROTOTYPES ===
tBinary	*Elf_Load(int fp);
 int	Elf_Relocate(void *Base);
 int	Elf_GetSymbol(void *Base, const char *Name, Uint *ret);
 int	Elf_Int_DoRelocate(Uint r_info, Uint32 *ptr, Uint32 addend, Elf32_Sym *symtab, Uint base);
Uint	Elf_Int_HashString(const char *str);

// === GLOBALS ===
tBinaryType	gELF_Info = {
	NULL,
	0x464C457F, 0xFFFFFFFF,	// '\x7FELF'
	"ELF",
	Elf_Load, Elf_Relocate, Elf_GetSymbol
	};

// === CODE ===
tBinary *Elf_Load(int fp)
{
	tBinary	*ret;
	Elf32_Ehdr	hdr;
	Elf32_Phdr	*phtab;
	 int	i, j;
	 int	iLoadCount;
	
	ENTER("xfp", fp);
	
	// Read ELF Header
	VFS_Read(fp, sizeof(hdr), &hdr);
	
	// Check the file type
	if(hdr.ident[0] != 0x7F || hdr.ident[1] != 'E' || hdr.ident[2] != 'L' || hdr.ident[3] != 'F') {
		Log_Warning("ELF", "Non-ELF File was passed to the ELF loader");
		LEAVE('n');
		return NULL;
	}
	
	// Check for a program header
	if(hdr.phoff == 0) {
		#if DEBUG_WARN
		Log_Warning("ELF", "File does not contain a program header (phoff == 0)");
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
	LOG("hdr.phoff = 0x%08x", hdr.phoff);
	VFS_Seek(fp, hdr.phoff, SEEK_SET);
	VFS_Read(fp, sizeof(Elf32_Phdr)*hdr.phentcount, phtab);
	
	// Count Pages
	iLoadCount = 0;
	LOG("hdr.phentcount = %i", hdr.phentcount);
	for( i = 0; i < hdr.phentcount; i++ )
	{
		// Ignore Non-LOAD types
		if(phtab[i].Type != PT_LOAD)
			continue;
		iLoadCount ++;
		LOG("phtab[%i] = {VAddr:0x%x, MemSize:0x%x}", i, phtab[i].VAddr, phtab[i].MemSize);
	}
	
	LOG("iLoadCount = %i", iLoadCount);
	
	// Allocate Information Structure
	ret = malloc( sizeof(tBinary) + sizeof(tBinarySection)*iLoadCount );
	// Fill Info Struct
	ret->Entry = hdr.entrypoint;
	ret->Base = -1;		// Set Base to maximum value
	ret->NumSections = iLoadCount;
	ret->Interpreter = NULL;
	
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
			if(ret->Interpreter)	continue;
			tmp = malloc(phtab[i].FileSize);
			VFS_Seek(fp, phtab[i].Offset, 1);
			VFS_Read(fp, phtab[i].FileSize, tmp);
			ret->Interpreter = Binary_RegInterp(tmp);
			LOG("Interpreter '%s'", tmp);
			free(tmp);
			continue;
		}
		// Ignore non-LOAD types
		if(phtab[i].Type != PT_LOAD)	continue;
		
		// Find Base
		if(phtab[i].VAddr < ret->Base)	ret->Base = phtab[i].VAddr;
		
		LOG("phtab[%i] = {VAddr:0x%x,Offset:0x%x,FileSize:0x%x}",
			i, phtab[i].VAddr, phtab[i].Offset, phtab[i].FileSize);
		
		ret->LoadSections[j].Offset = phtab[i].Offset;
		ret->LoadSections[j].FileSize = phtab[i].FileSize;
		ret->LoadSections[j].Virtual = phtab[i].VAddr;
		ret->LoadSections[j].MemSize = phtab[i].MemSize;
		ret->LoadSections[j].Flags = 0;
		if( !(phtab[i].Flags & PF_W) )
			ret->LoadSections[j].Flags |= BIN_SECTFLAG_RO;
		if( phtab[i].Flags & PF_X )
			ret->LoadSections[j].Flags |= BIN_SECTFLAG_EXEC;
		j ++;
	}
	
	// Clean Up
	free(phtab);
	// Return
	LEAVE('p', ret);
	return ret;
}

// --- ELF RELOCATION ---
// Taken from 'ld-acess.so'
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
	Uint	iRealBase = -1;
	Uint	iBaseDiff;
	 int	iSegmentCount;
	 int	iSymCount = 0;
	Elf32_Rel	*rel = NULL;
	Elf32_Rela	*rela = NULL;
	Uint32	*pltgot = NULL;
	void	*plt = NULL;
	Uint32	*ptr;
	 int	relSz=0, relEntSz=8;
	 int	relaSz=0, relaEntSz=8;
	 int	pltSz=0, pltType=0;
	Elf32_Dyn	*dynamicTab = NULL;	// Dynamic Table Pointer
	char	*dynstrtab = NULL;	// .dynamic String Table
	Elf32_Sym	*dynsymtab = NULL;
	 int	bFailed = 0;
	
	ENTER("pBase", Base);
	
	// Parse Program Header to get Dynamic Table
	phtab = (void *)( (tVAddr)Base + hdr->phoff );
	iSegmentCount = hdr->phentcount;
	for(i = 0; i < iSegmentCount; i ++ )
	{
		// Determine linked base address
		if(phtab[i].Type == PT_LOAD && iRealBase > phtab[i].VAddr)
			iRealBase = phtab[i].VAddr;
		
		// Find Dynamic Section
		if(phtab[i].Type == PT_DYNAMIC) {
			if(dynamicTab) {
				Log_Warning("ELF", "Elf_Relocate - Multiple PT_DYNAMIC segments\n");
				continue;
			}
			dynamicTab = (void *) (tVAddr) phtab[i].VAddr;
			j = i;	// Save Dynamic Table ID
			break;
		}
	}
	
	// Check if a PT_DYNAMIC segement was found
	if(!dynamicTab) {
		Log_Warning("ELF", "Elf_Relocate: No PT_DYNAMIC segment in image, returning\n");
		LEAVE('x', hdr->entrypoint);
		return hdr->entrypoint;
	}
	
	// Page Align real base
	iRealBase &= ~0xFFF;
	
	// Adjust "Real" Base
	iBaseDiff = (Uint)Base - iRealBase;
	// Adjust Dynamic Table
	dynamicTab = (void *) ((Uint)dynamicTab + iBaseDiff);
	
	// === Get Symbol table and String Table ===
	for( j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
	{
		switch(dynamicTab[j].d_tag)
		{
		// --- Symbol Table ---
		case DT_SYMTAB:
			dynamicTab[j].d_val += iBaseDiff;
			dynsymtab = (void*) (tVAddr) dynamicTab[j].d_val;
			hdr->misc.SymTable = dynamicTab[j].d_val;	// Saved in unused bytes of ident
			break;
		
		// --- String Table ---
		case DT_STRTAB:
			dynamicTab[j].d_val += iBaseDiff;
			dynstrtab = (void*) (tVAddr) dynamicTab[j].d_val;
			break;
		
		// --- Hash Table --
		case DT_HASH:
			dynamicTab[j].d_val += iBaseDiff;
			iSymCount = ((Uint*)((tVAddr)dynamicTab[j].d_val))[1];
			hdr->misc.HashTable = dynamicTab[j].d_val;	// Saved in unused bytes of ident
			break;
		}
	}

	if( !dynsymtab && iSymCount > 0 ) {
		Log_Warning("ELF", "Elf_Relocate: No Dynamic symbol table, but count >0");
		return 0;
	}

	// Alter Symbols to true base
	for(i = 0; i < iSymCount; i ++)
	{
		dynsymtab[i].value += iBaseDiff;
		dynsymtab[i].nameOfs += (Uint)dynstrtab;
		//LOG("Sym '%s' = 0x%x (relocated)\n", dynsymtab[i].name, dynsymtab[i].value);
	}
	
	// === Add to loaded list (can be imported now) ===
	//Binary_AddLoaded( (Uint)Base );

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
			Log_Notice("ELF", "%p - Required Library '%s' (Ignored in kernel mode)\n", Base, libPath);
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
			if( !Elf_Int_DoRelocate(rel[i].r_info, ptr, *ptr, dynsymtab, (Uint)Base) ) {
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
			if( !Elf_Int_DoRelocate(rela[i].r_info, ptr, rela[i].r_addend, dynsymtab, (Uint)Base) ) {
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
				if( !Elf_Int_DoRelocate(pltRel[i].r_info, ptr, *ptr, dynsymtab, (Uint)Base) ) {
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
				if( !Elf_Int_DoRelocate(pltRela[i].r_info, ptr, pltRela[i].r_addend, dynsymtab, (Uint)Base) ) {
					bFailed = 1;
				}
			}
		}
	}
	
	if(bFailed) {
		LEAVE('i', 0);
		return 0;
	}
	
	LEAVE('x', 1);
	return 1;
}

/**
 * \fn void Elf_Int_DoRelocate(Uint r_info, Uint32 *ptr, Uint32 addend, Elf32_Sym *symtab, Uint base)
 * \brief Performs a relocation
 * \param r_info	Field from relocation entry
 * \param ptr	Pointer to location of relocation
 * \param addend	Value to add to symbol
 * \param symtab	Symbol Table
 * \param base	Base of loaded binary
 */
int Elf_Int_DoRelocate(Uint r_info, Uint32 *ptr, Uint32 addend, Elf32_Sym *symtab, Uint base)
{
	Uint	val;
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
		LOG("%08x R_386_PC32 *0x%x = 0x%x + 0x%x('%s') - 0x%x", r_info, ptr, *ptr, val, sSymName, (Uint)ptr );
		// TODO: Check if it needs the true value of ptr or the compiled value
		// NOTE: Testing using true value
		*ptr = val + addend - (Uint)ptr;
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
		LOG("%08x R_386_JMP_SLOT *0x%x = 0x%x (%s)", r_info, ptr, val, sSymName);
		*ptr = val;
		break;

	// Base Address (B+A)
	case R_386_RELATIVE:
		LOG("%08x R_386_RELATIVE *0x%x = 0x%x + 0x%x", r_info, ptr, base, addend);
		*ptr = base + addend;
		break;
		
	default:
		LOG("Rel 0x%x: 0x%x,%i", ptr, sym, type);
		break;
	}
	return 1;
}

/**
 * \fn int Elf_GetSymbol(void *Base, const char *name, Uint *ret)
 * \brief Get a symbol from the loaded binary
 */
int Elf_GetSymbol(void *Base, const char *Name, Uint *ret)
{
	Elf32_Ehdr	*hdr = (void*)Base;
	Elf32_Sym	*symtab;
	 int	nbuckets = 0;
	 int	iSymCount = 0;
	 int	i;
	Uint	*pBuckets;
	Uint	*pChains;
	Uint	iNameHash;

	if(!Base)	return 0;

	pBuckets = (void *) hdr->misc.HashTable;
	symtab = (void *) hdr->misc.SymTable;
	
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
 * \fn Uint Elf_Int_HashString(char *str)
 * \brief Hash a string in the ELF format
 * \param str	String to hash
 * \return Hash value
 */
Uint Elf_Int_HashString(const char *str)
{
	Uint	h = 0, g;
	while(*str)
	{
		h = (h << 4) + *str++;
		if( (g = h & 0xf0000000) )
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}
