/*
 * Acess2
 *
 * ARM7 Entrypoint
 * arch/arm7/main.c
 */
#include <acess.h>
#include <modules.h>

// === IMPORTS ===
extern void	Interrupts_Setup(void);
extern void	Arch_LoadBootModules(void);
extern void	Heap_Install(void);
extern void	Threads_Init(void);
extern void	System_Init(const char *Commandline);

// === PROTOTYPES ===
 int	kmain(void);

// === CODE ===
int kmain(void)
{
	LogF("Acess2 ARMv7 v"EXPAND_STR(KERNEL_VERSION)"\n");
	LogF(" Build %i\n", BUILD_NUM);
//	Interrupts_Setup();
	
	MM_SetupPhys();

	LogF("Heap Setup...\n");
	Heap_Install();

	LogF("Threads Init...\n");
	Threads_Init();
	
	LogF("VFS Init...\n");
	VFS_Init();

	// Boot modules?
	Module_EnsureLoaded("armv7_GIC");

	//
	LogF("Moving to arch-independent init\n");
	System_Init("");
	//TODO: 
	LogF("End of kmain(), for(;;) Threads_Sleep();\n");
	for(;;)
		Threads_Sleep();
}

void Arch_LoadBootModules(void)
{
}

