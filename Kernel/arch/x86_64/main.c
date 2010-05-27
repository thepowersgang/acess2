/*
 * Acess2 x86_64 Project
 */
#include <acess.h>

// === IMPORTS ===
extern void	Desctab_Init(void);
extern void	MM_InitVirt(void);

// === PROTOTYPES ===

// === GLOBALS ==

// === CODE ===
void kmain(Uint MbMagic, void *MbInfoPtr)
{
	*(Uint16*)(0xB8000) = 0x1F00|'A';
	
	Desctab_Init();
	MM_InitVirt();
	
	for(;;)
		__asm__ __volatile__ ("hlt");
}

void Arch_LoadBootModules(void)
{
	
}

void StartupPrint(char *String)
{
	
}
