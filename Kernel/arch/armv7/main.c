/*
 * Acess2
 *
 * ARM7 Entrypoint
 * arch/arm7/main.c
 */
#define DEBUG	0

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
Uint32	ARMv7_int_HandleSyscalls(Uint32 Num, Uint32 *Args);

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
	System_Init("Acess2.armv7.bin /Acess=initrd: -VTerm:Video=PL110");
//	System_Init("/Acess=initrd:");
	//TODO: 
	LogF("End of kmain(), for(;;) Threads_Sleep();\n");
	for(;;)
		Threads_Sleep();
}

void Arch_LoadBootModules(void)
{
}

Uint32 ARMv7_int_HandleSyscalls(Uint32 Num, Uint32 *Args)
{
	Uint32	ret = -1, err = 0;
	Uint32	addr;
	ENTER("iNum xArgs[0] xArgs[1] xArgs[2] xArgs[3]",
		Num, Args[0], Args[1], Args[2], Args[3]
		);
	switch(Num)
	{
	case 1:
//		Log_Debug("ARMv7", "__clear_cache(%p, %p)", Args[0], Args[1]);
		// Align
		Args[0] &= ~0xFFF;
		Args[1] += 0xFFF;	Args[1] &= ~0xFFF;
		// Invalidate!
		for( addr = Args[0]; addr < Args[1]; addr += 0x1000 )
		{
			LOG("addr = %p", addr);
			__asm__ __volatile__ (
				"mcrlt p15, 0, %0, c7, c5, 1;\n\t"
				"mcrlt p15, 0, %0, c7, c6, 1;\n\t"
				:
				: "r" (addr)
				);
		}
		ret = 0;
		break;
	}
	Args[0] = ret;	// RetLow
	Args[1] = 0;	// RetHi
	Args[2] = err;	// Errno
	LEAVE('x', ret);
	return ret;
}

