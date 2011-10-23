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
#include "elf32.h"

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
void	*Elf32Load(int FD, Elf32_Ehdr *hdr);

// === CODE ===
void *Elf_Load(int FD)
{
	Elf32_Ehdr	hdr;
	
	// Read ELF Header
	acess_read(FD, sizeof(hdr), &hdr);
	
	// Check the file type
	if(hdr.e_ident[0] != 0x7F || hdr.e_ident[1] != 'E' || hdr.e_ident[2] != 'L' || hdr.e_ident[3] != 'F') {
		Warning("Non-ELF File was passed to the ELF loader\n");
		return NULL;
	}

	switch(hdr.e_ident[4])
	{
	case ELFCLASS32:
		return Elf32Load(FD, &hdr);
	default:
		return NULL;
	}
}
void *Elf32Load(int FD, Elf32_Ehdr *hdr)
{
	Elf32_Phdr	*phtab;
	 int	i;
	 int	iPageCount;
	uint32_t	max, base;
	uint32_t	addr;
	uint32_t	baseDiff = 0;
	
	ENTER("iFD", FD);
	
	// Check for a program header
	if(hdr->phoff == 0) {
		#if DEBUG_WARN
		Warning("ELF File does not contain a program header\n");
		#endif
		LEAVE('n');
		return NULL;
	}
	
	// Read Program Header Table
	phtab = malloc( sizeof(Elf32_Phdr) * hdr->phentcount );
	if( !phtab ) {
		LEAVE('n');
		return NULL;
	}
	LOG("hdr.phoff = 0x%08x\n", hdr->phoff);
	acess_seek(FD, hdr->phoff, ACESS_SEEK_SET);
	acess_read(FD, sizeof(Elf32_Phdr) * hdr->phentcount, phtab);
	
	// Count Pages
	iPageCount = 0;
	LOG("hdr.phentcount = %i\n", hdr->phentcount);
	for( i = 0; i < hdr->phentcount; i++ )
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
	for( i = 0; i < hdr->phentcount; i ++)
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
	for( i = 0; i < hdr->phentcount; i++ )
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
