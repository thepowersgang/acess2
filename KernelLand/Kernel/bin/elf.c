/*
 * Acess v0.1
 * ELF Executable Loader Code
 */
#define DEBUG	0
#include <acess.h>
#include <binary.h>

#define _COMMON_H
#define SysDebug(v...)	LOG(v)
#define DISABLE_ELF64
void	*GetSymbol(const char *Name, size_t *Size);
void	*GetSymbol(const char *Name, size_t *Size) { Uint val; Binary_GetSymbol(Name, &val); if(Size)*Size=0; return (void*)val; };
#define AddLoaded(a,b)	do{}while(0)
#define LoadLibrary(a,b,c)	0
#include "../../../Usermode/Libraries/ld-acess.so_src/elf.c"

#define DEBUG_WARN	1

// === PROTOTYPES ===
tBinary	*Elf_Load(int fp);
tBinary	*Elf_Load64(int fp, Elf64_Ehdr *hdr);
tBinary	*Elf_Load32(int fp, Elf32_Ehdr *hdr);
 int	Elf_Relocate(void *Base);
 int	Elf_GetSymbol(void *Base, const char *Name, Uint *Ret);
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
	Elf64_Ehdr	hdr;
	
	// Read ELF Header
	VFS_Read(fp, sizeof(hdr), &hdr);
	
	// Check the file type
	if(hdr.e_ident[0] != 0x7F || hdr.e_ident[1] != 'E' || hdr.e_ident[2] != 'L' || hdr.e_ident[3] != 'F') {
		Log_Warning("ELF", "Non-ELF File was passed to the ELF loader");
		return NULL;
	}

	switch(hdr.e_ident[4])	// EI_CLASS
	{
	case ELFCLASS32:
		return Elf_Load32(fp, (void*)&hdr);
	case ELFCLASS64:
		return Elf_Load64(fp, &hdr);
	default:
		Log_Warning("ELF", "Unknown EI_CLASS value %i", hdr.e_ident[4]);
		return NULL;
	}
}

tBinary *Elf_Load64(int FD, Elf64_Ehdr *Header)
{
	tBinary	*ret;
	Elf64_Phdr	phtab[Header->e_phnum];
	 int	nLoadSegments;
	 int	i, j;
	
	// Sanity check
	if( Header->e_phoff == 0 )
	{
		Log_Warning("ELF", "No program header, panic!");
		return NULL;
	}
	if( Header->e_shentsize != sizeof(Elf64_Shdr) ) {
		Log_Warning("ELF", "Header gives shentsize as %i, my type is %i",
			Header->e_shentsize, sizeof(Elf64_Shdr) );
	}
	if( Header->e_phentsize != sizeof(Elf64_Phdr) ) {
		Log_Warning("ELF", "Header gives phentsize as %i, my type is %i",
			Header->e_phentsize, sizeof(Elf64_Phdr) );
	}

	LOG("Header = {");
	LOG("  e_ident = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
		Header->e_ident[0], Header->e_ident[1], Header->e_ident[2], Header->e_ident[3],
		Header->e_ident[4], Header->e_ident[5], Header->e_ident[6], Header->e_ident[7],
		Header->e_ident[8], Header->e_ident[9], Header->e_ident[10], Header->e_ident[11],
		Header->e_ident[12], Header->e_ident[13], Header->e_ident[14], Header->e_ident[15]
		);
	LOG("  e_type = %i", Header->e_type);
	LOG("  e_machine = %i", Header->e_machine);
	LOG("  e_version = %i", Header->e_version);
	LOG("  e_entry   = 0x%llx", Header->e_entry);
	LOG("  e_phoff   = 0x%llx", Header->e_phoff);
	LOG("  e_shoff   = 0x%llx", Header->e_shoff);
	LOG("  e_flags   = 0x%x", Header->e_flags);
	LOG("  e_ehsize  = %i", Header->e_ehsize);
	LOG("  e_phentsize = %i", Header->e_phentsize);
	LOG("  e_phnum   = %i", Header->e_phnum);
	LOG("  e_shentsize = %i", Header->e_shentsize);
	LOG("  e_shnum   = %i", Header->e_shnum);
	LOG("  e_shstrndx = %i", Header->e_shstrndx);
	LOG("}");

	// Load Program Header table
	VFS_Seek(FD, Header->e_phoff, SEEK_SET);
	VFS_Read(FD, sizeof(Elf64_Phdr)*Header->e_phnum, phtab);

	// Count load segments
	nLoadSegments = 0;
	for( i = 0; i < Header->e_phnum; i ++ )
	{
		if( phtab[i].p_type != PT_LOAD )	continue ;
		nLoadSegments ++;
	}
	
	// Allocate Information Structure
	ret = malloc( sizeof(tBinary) + sizeof(tBinarySection)*nLoadSegments );
	// Fill Info Struct
	ret->Entry = Header->e_entry;
	ret->Base = -1;		// Set Base to maximum value
	ret->NumSections = nLoadSegments;
	ret->Interpreter = NULL;

	j = 0;	// LoadSections[] index
	for( i = 0; i < Header->e_phnum; i ++ )
	{
		LOG("phtab[%i] = {", i);
		LOG("  .p_type   = %i", phtab[i].p_type);
		LOG("  .p_flags  = 0x%x", phtab[i].p_flags);
		LOG("  .p_offset = 0x%llx", phtab[i].p_offset);
		LOG("  .p_vaddr  = 0x%llx", phtab[i].p_vaddr);
		LOG("  .p_paddr  = 0x%llx", phtab[i].p_paddr);
		LOG("  .p_filesz = 0x%llx", phtab[i].p_filesz);
		LOG("  .p_memsz  = 0x%llx", phtab[i].p_memsz);
		LOG("  .p_align  = 0x%llx", phtab[i].p_align);
		LOG("}");

		// Get Interpreter Name
		if( phtab[i].p_type == PT_INTERP )
		{
			char *tmp;
			if(ret->Interpreter)	continue;
			tmp = malloc(phtab[i].p_filesz);
			VFS_Seek(FD, phtab[i].p_offset, 1);
			VFS_Read(FD, phtab[i].p_filesz, tmp);
			ret->Interpreter = Binary_RegInterp(tmp);
			LOG("Interpreter '%s'", tmp);
			free(tmp);
			continue;
		}
		
		if( phtab[i].p_type != PT_LOAD )	continue ;
		
		// Find the executable base
		if( phtab[i].p_vaddr < ret->Base )	ret->Base = phtab[i].p_vaddr;

		ret->LoadSections[j].Offset = phtab[i].p_offset;
		ret->LoadSections[j].Virtual = phtab[i].p_vaddr;
		ret->LoadSections[j].FileSize = phtab[i].p_filesz;
		ret->LoadSections[j].MemSize = phtab[i].p_memsz;
		
		ret->LoadSections[j].Flags = 0;
		if( !(phtab[i].p_flags & PF_W) )
			ret->LoadSections[j].Flags |= BIN_SECTFLAG_RO;
		if( phtab[i].p_flags & PF_X )
			ret->LoadSections[j].Flags |= BIN_SECTFLAG_EXEC;
		j ++;
	}

	return ret;
}

tBinary *Elf_Load32(int FD, Elf32_Ehdr *Header)
{
	tBinary	*ret;
	Elf32_Phdr	*phtab;
	 int	i, j;
	 int	iLoadCount;

	ENTER("xFD", FD);

	// Check architecture with current CPU
	// - TODO: Support kernel level emulation
	#if ARCH_IS_x86
	if( Header->machine != EM_386 )
	{
		Log_Warning("ELF", "Unknown architecure on ELF-32");
		LEAVE_RET('n');
		return NULL;
	}
	#endif

	// Check for a program header
	if(Header->phoff == 0) {
		#if DEBUG_WARN
		Log_Warning("ELF", "File does not contain a program header (phoff == 0)");
		#endif
		LEAVE('n');
		return NULL;
	}
	
	// Read Program Header Table
	phtab = malloc( sizeof(Elf32_Phdr) * Header->phentcount );
	if( !phtab ) {
		LEAVE('n');
		return NULL;
	}
	LOG("hdr.phoff = 0x%08x", Header->phoff);
	VFS_Seek(FD, Header->phoff, SEEK_SET);
	VFS_Read(FD, sizeof(Elf32_Phdr)*Header->phentcount, phtab);
	
	// Count Pages
	iLoadCount = 0;
	LOG("Header->phentcount = %i", Header->phentcount);
	for( i = 0; i < Header->phentcount; i++ )
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
	ret->Entry = Header->entrypoint;
	ret->Base = -1;		// Set Base to maximum value
	ret->NumSections = iLoadCount;
	ret->Interpreter = NULL;
	
	// Load Pages
	j = 0;
	for( i = 0; i < Header->phentcount; i++ )
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
			VFS_Seek(FD, phtab[i].Offset, 1);
			VFS_Read(FD, phtab[i].FileSize, tmp);
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

int Elf_Relocate(void *Base)
{
	return  ElfRelocate(Base, (char**){NULL}, "") != NULL;
}
int Elf_GetSymbol(void *Base, const char *Name, Uint *ret)
{
	return ElfGetSymbol(Base, Name, (void**)ret, NULL);
}

