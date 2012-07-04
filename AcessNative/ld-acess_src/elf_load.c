/*
 * Acess2 - AcessNative
 *
 * ELF Executable Loader Code
 */
#define DEBUG	0
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "elf32.h"
#include "elf64.h"

#define DEBUG_WARN	1

#define MKPTR(_type,_val)	((_type*)(uintptr_t)(_val))
#define PTRMK(_type,_val)	MKPTR(_type,_val)
#define PTR(_val)	((void*)(uintptr_t)(_val))

#if DEBUG
# define ENTER(...)	printf("%s: ---- ENTER ----\n", __func__);
# define LOG(s, ...)	printf("%s: " s, __func__, __VA_ARGS__)
# define LOGS(s)	printf("%s: " s, __func__)
# define LEAVE(...)	printf("%s: ---- LEAVE ----\n", __func__);
#else
# define ENTER(...)
# define LOG(...)
# define LOGS(...)
# define LEAVE(...)
#endif

// === PROTOTYPES ===
void	*Elf_Load(int FD);
void	*Elf32Load(int FD, Elf32_Ehdr *hdr);
void	*Elf64Load(int FD, Elf64_Ehdr *hdr);

// === CODE ===
void *Elf_Load(int FD)
{
	Elf64_Ehdr	hdr;
	
	// Read ELF Header
	acess_read(FD, &hdr, sizeof(hdr));
	
	// Check the file type
	if(hdr.e_ident[0] != 0x7F || hdr.e_ident[1] != 'E' || hdr.e_ident[2] != 'L' || hdr.e_ident[3] != 'F') {
		Warning("Non-ELF File was passed to the ELF loader\n");
		return NULL;
	}

	switch(hdr.e_ident[4])
	{
	case ELFCLASS32:
		return Elf32Load(FD, (void*)&hdr);
	case ELFCLASS64:
		return Elf64Load(FD, &hdr);
	default:
		Warning("Unknown ELF class (%i)", hdr.e_ident[4]);
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
	acess_read(FD, phtab, sizeof(Elf32_Phdr) * hdr->phentcount);
	
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
		// Find a nice space (47 address bits allowed)
		base = FindFreeRange( max, 47 );
		LOG("new base = %08x\n", base);
		if( base == 0 )	return NULL;
		baseDiff = base;
	}
	
	// Load Pages
	for( i = 0; i < hdr->phentcount; i++ )
	{
		// Get Interpreter Name
		if( phtab[i].Type == PT_INTERP )
		{
			char *tmp;
			//if(ret->Interpreter)	continue;
			tmp = malloc(phtab[i].FileSize);
			acess_seek(FD, phtab[i].Offset, ACESS_SEEK_SET);
			acess_read(FD, tmp, phtab[i].FileSize);
			//ret->Interpreter = Binary_RegInterp(tmp);
			LOG("Interpreter '%s'\n", tmp);
			free(tmp);
			continue;
		}
		// Ignore non-LOAD types
		if(phtab[i].Type != PT_LOAD)	continue;
		
		LOG("phtab[%i] = PT_LOAD {Adj VAddr:0x%x, Offset:0x%x, FileSize:0x%x, MemSize:0x%x}\n",
			i, phtab[i].VAddr+baseDiff, phtab[i].Offset, phtab[i].FileSize, phtab[i].MemSize);
		
		addr = phtab[i].VAddr + baseDiff;

		if( AllocateMemory( addr, phtab[i].MemSize ) ) {
			fprintf(stderr, "Elf_Load: Unable to map memory at %x (0x%x bytes)\n",
				addr, phtab[i].MemSize);
			free( phtab );
			return NULL;
		}
		
		acess_seek(FD, phtab[i].Offset, ACESS_SEEK_SET);
		acess_read(FD, PTRMK(void, addr), phtab[i].FileSize);
		memset( PTRMK(char, addr) + phtab[i].FileSize, 0, phtab[i].MemSize - phtab[i].FileSize );
	}
	
	// Clean Up
	free(phtab);
	// Return
	LEAVE('p', base);
	return PTRMK(void, base);
}

void *Elf64Load(int FD, Elf64_Ehdr *hdr)
{
	Elf64_Phdr	*phtab;
	 int	i;
	 int	iPageCount;
	uint64_t	max, base;
	uint64_t	addr;
	uint64_t	baseDiff = 0;
	
	ENTER("iFD", FD);
	
	if( sizeof(void*) == 4) {
		Warning("ELF64 being loaded in 32-bit env, this may not work");
	}

	// Check for a program header
	if(hdr->e_phoff == 0) {
		#if DEBUG_WARN
		Warning("ELF File does not contain a program header\n");
		#endif
		LEAVE('n');
		return NULL;
	}
	
	// Read Program Header Table
	phtab = malloc( sizeof(Elf64_Phdr) * hdr->e_phnum );
	if( !phtab ) {
		LEAVE('n');
		return NULL;
	}
	LOG("hdr.phoff = 0x%08llx\n", (long long)hdr->e_phoff);
	acess_seek(FD, hdr->e_phoff, ACESS_SEEK_SET);
	acess_read(FD, phtab, sizeof(Elf64_Phdr) * hdr->e_phnum);
	
	// Count Pages
	iPageCount = 0;
	LOG("hdr.phentcount = %i\n", hdr->e_phnum);
	for( i = 0; i < hdr->e_phnum; i++ )
	{
		// Ignore Non-LOAD types
		if(phtab[i].p_type != PT_LOAD)
			continue;
		iPageCount += ((phtab[i].p_vaddr&0xFFF) + phtab[i].p_memsz + 0xFFF) >> 12;
		LOG("phtab[%i] = {VAddr:0x%llx, MemSize:0x%llx}\n",
			i, (long long)phtab[i].p_vaddr, (long long)phtab[i].p_memsz);
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
	for( i = 0; i < hdr->e_phnum; i ++)
	{
		if( phtab[i].p_type != PT_LOAD )
			continue;
		if( phtab[i].p_vaddr < base )
			base = phtab[i].p_vaddr;
		if( phtab[i].p_vaddr + phtab[i].p_memsz > max )
			max = phtab[i].p_vaddr + phtab[i].p_memsz;
	}

	LOG("base = %08lx, max = %08lx\n", base, max);

	if( base == 0 ) {
		// Find a nice space (31 address bits allowed)
		base = FindFreeRange( max, 31 );
		LOG("new base = %08lx\n", base);
		if( base == 0 )	return NULL;
		baseDiff = base;
	}
	
	// Load Pages
	for( i = 0; i < hdr->e_phnum; i++ )
	{
		// Get Interpreter Name
		if( phtab[i].p_type == PT_INTERP )
		{
			char *tmp;
			//if(ret->Interpreter)	continue;
			tmp = malloc(phtab[i].p_filesz+1);
			tmp[ phtab[i].p_filesz ] = 0;
			acess_seek(FD, phtab[i].p_offset, ACESS_SEEK_SET);
			acess_read(FD, tmp, phtab[i].p_filesz);
			//ret->Interpreter = Binary_RegInterp(tmp);
			LOG("Interpreter '%s'\n", tmp);
			free(tmp);
			continue;
		}
		// Ignore non-LOAD types
		if(phtab[i].p_type != PT_LOAD)	continue;
		
		LOG("phtab[%i] = PT_LOAD {Adj VAddr:0x%llx, Offset:0x%llx, FileSize:0x%llx, MemSize:0x%llx}\n",
			i,
			(long long)phtab[i].p_vaddr+baseDiff, (long long)phtab[i].p_offset,
			(long long)phtab[i].p_filesz, (long long)phtab[i].p_memsz
			);
		
		addr = phtab[i].p_vaddr + baseDiff;

		if( AllocateMemory( addr, phtab[i].p_memsz ) ) {
			fprintf(stderr, "Elf_Load: Unable to map memory at %llx (0x%llx bytes)\n",
				(long long)addr, (long long)phtab[i].p_memsz);
			free( phtab );
			return NULL;
		}
		
		acess_seek(FD, phtab[i].p_offset, ACESS_SEEK_SET);
		acess_read(FD, PTRMK(void, addr), phtab[i].p_filesz);
		memset( PTRMK(char, addr) + phtab[i].p_filesz, 0, phtab[i].p_memsz - phtab[i].p_filesz );
	}
	
	// Clean Up
	free(phtab);
	// Return
	LEAVE('p', base);
	return PTRMK(void, base);
}

