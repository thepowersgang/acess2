/*
 AcessOS 1 - Dynamic Loader
 By thePowersGang
*/
#include <stdint.h>
#include <stddef.h>
#include "common.h"

// === PROTOTYPES ===
void	*DoRelocate(void *base, char **envp, const char *Filename);
 int	CallUser(void *Entry, void *SP);

// === Imports ===
extern char	gLinkedBase[];
char	**gEnvP;
extern int	memcmp(const void *m1, const void *m2, size_t size);
 
// === CODE ===
/**
 \fn int SoMain(Uint base, int argc, char *argv[], char **envp)
 \brief Library entry point
 \note This is the entrypoint for the library
*/
void *SoMain(void *base, int argc, char **argv, char **envp)
{
	void	*ret;
	 
	gEnvP = envp;

	// - Assume that the file pointer will be less than 4096
	if((intptr_t)base < 0x1000) {
		SysDebug("ld-acess - SoMain: Passed file pointer %i\n", base);
		_exit(-1);
		for(;;);
	}
	// Check if we are being called directly
	if(base == &gLinkedBase) {
		SysDebug("ld-acess should not be directly called\n");
		_exit(1);
		for(;;);
	}

	// Otherwise do relocations
	//ret = DoRelocate( base, envp, "Executable" );
	ret = DoRelocate( base, NULL, "Executable" );
	if( ret == 0 ) {
		SysDebug("ld-acess - SoMain: Relocate failed, base=0x%x\n", base);
		_exit(-1);
		for(;;);
	}

	SysDebug("ld-acess - SoMain: ret = %p", ret);	
	return ret;
}

/**
 \fn int DoRelocate(void *base, char **envp)
 \brief Relocates an in-memory image
*/
void *DoRelocate(void *base, char **envp, const char *Filename)
{
	uint8_t	*hdr = base;
	// Load Executable
	if(memcmp(base, "\x7F""ELF", 4) == 0)
		return ElfRelocate(base, envp, Filename);
	if(hdr[0] == 0x7F && hdr[1] == 'E' && hdr[2] == 'L' && hdr[3] == 'F')
		return ElfRelocate(base, envp, Filename);

	if(hdr[0] == 'M' && hdr[1] == 'Z')
		return PE_Relocate(base, envp, Filename);
	
	SysDebug("ld-acess - DoRelocate: Unkown file format '0x%x 0x%x 0x%x 0x%x'\n",
		hdr[0], hdr[1], hdr[2], hdr[3] );
	SysDebug("ld-acess - DoRelocate: File '%s'\n", Filename);
	_exit(-1);
	for(;;);
}

/**
 \fn int CallUser(Uint entry, Uint sp)
*/
int CallUser(void *entry, void *sp)
{
	#if ARCHDIR_IS_x86_64
	((void **)sp)[-1] = 0;	// Clear return address
	__asm__ __volatile__ (
	"mov %%rax, %%rsp;\n\t"
	"jmp *%%rcx"
	: : "a"(sp), "c"(entry));
	#elif ARCHDIR_IS_x86
	((void **)sp)[-1] = 0;	// Clear return address
	__asm__ __volatile__ (
	"mov %%eax, %%esp;\n\t"
	"jmp *%%ecx"
	: : "a"(sp), "c"(entry));
	#endif
	for(;;);
}

void abort(void)
{
	_exit(-4);
}
