/*
 AcessOS 1 - Dynamic Loader
 By thePowersGang
*/
#include "common.h"

// === PROTOTYPES ===
 int	DoRelocate( Uint base, char **envp, char *Filename );
 int	CallUser(Uint entry, Uint SP);
 int	ElfRelocate(void *Base, char *envp[], char *Filename);
 int	PE_Relocate(void *Base, char *envp[], char *Filename);

// === Imports ===
extern void	gLinkedBase;
extern tLoadedLib	gLoadedLibraries[];
 
// === CODE ===
/**
 \fn int SoMain(Uint base, int argc, char *argv[], char **envp)
 \brief Library entry point
 \note This is the entrypoint for the library
*/
int SoMain(Uint base, int arg1)
{
	 int	ret;
	 
	// - Assume that the file pointer will be less than 4096
	if(base < 0x1000) {
		SysDebug("ld-acess - SoMain: Passed file pointer %i\n", base);
		_exit(-1);
		for(;;);
	}
	// Check if we are being called directly
	if(base == (Uint)&gLinkedBase) {
		SysDebug("ld-acess should not be directly called\n");
		_exit(1);
		for(;;);
	}

	gLoadedLibraries[0].Base = (Uint)&gLinkedBase;
	gLoadedLibraries[0].Name = "ld-acess.so";
	
	// Otherwise do relocations
	//ret = DoRelocate( base, envp, "Executable" );
	ret = DoRelocate( base, NULL, "Executable" );
	if( ret == 0 ) {
		SysDebug("ld-acess - SoMain: Relocate failed, base=0x%x\n", base);
		_exit(-1);
		for(;;);
	}
	
	// And call user
	//SysDebug("Calling User at 0x%x\n", ret);
	CallUser( ret, (Uint)&arg1 );
	
	return 0;
}

/**
 \fn int DoRelocate(Uint base, char **envp)
 \brief Relocates an in-memory image
*/
int DoRelocate( Uint base, char **envp, char *Filename )
{
	// Load Executable
	if(*(Uint*)base == (0x7F|('E'<<8)|('L'<<16)|('F'<<24)))
		return ElfRelocate((void*)base, envp, Filename);
	if(*(Uint16*)base == ('M'|('Z'<<8)))
		return PE_Relocate((void*)base, envp, Filename);
	
	SysDebug("ld-acess - DoRelocate: Unkown file format '0x%x 0x%x 0x%x 0x%x'\n",
		*(Uint8*)(base), *(Uint8*)(base+1), *(Uint8*)(base+2), *(Uint8*)(base+3) );
	SysDebug("ld-acess - DoRelocate: File '%s'\n", Filename);
	_exit(-1);
	for(;;);
}

/**
 \fn int CallUser(Uint entry, Uint sp)
*/
int CallUser(Uint entry, Uint sp)
{
	//SysDebug("CallUser: (entry=0x%x, sp=0x%x)", entry, sp);
	*(Uint*)(sp-4) = 0;	// Clear return address
	__asm__ __volatile__ (
	"mov %%eax, %%esp;\n\t"
	"jmp *%%ecx"
	: : "a"(sp), "c"(entry));
	for(;;);
}
