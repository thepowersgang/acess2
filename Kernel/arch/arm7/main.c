/*
 * Acess2
 *
 * ARM7 Entrypoint
 * arch/arm7/main.c
 */
#include <acess.h>

// === IMPORTS ===
extern void	Interrupts_Setup(void);
extern void	Arch_LoadBootModules(void);
extern void	Heap_Install(void);
extern void	Threads_Init(void);

// === PROTOTYPES ===
 int	kmain(void);

// === CODE ===
int kmain(void)
{
	LogF("Acess2 ARMv7 v"EXPAND_STR(KERNEL_VERSION)"\n", BUILD_NUM);
	LogF(" Build %i\n", BUILD_NUM);
//	Interrupts_Setup();
	
	MM_SetupPhys();

	Heap_Install();

	Threads_Init();
	
	//TODO: 
	LogF("End of kmain(), for(;;);\n");
	for(;;);
}

void Arch_LoadBootModules(void)
{
}

