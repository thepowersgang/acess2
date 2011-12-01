/*
 * Acess2 M68K port
 * - By John Hodge (thePowersGang)
 *
 * arch/m68k/main.c
 * - C entrypoint
 */
#include <acess.h>
#include <init.h>

// === PROTOTYPES ===
void	kmain(void);

// === CODE ===
void kmain(void)
{
	LogF("Acess2 m68k v"EXPAND_STR(KERNEL_VERSION)"\n");
	LogF(" Build %i, Git Hash %s\n", BUILD_NUM, gsGitHash);
	
}

void Arch_LoadBootModules(void)
{
	
}

