/*
 * Acess 2
 * x86 Kernel Main
 * arch/x86/main.c
 */
#include <acess.h>
#include <mboot.h>
#include <init.h>
#include <mm_virt.h>
#include <mp.h>

#define	VGA_ERRORS	0

// === IMPORTS ===
extern void	Heap_Install(void);
extern void	Desctab_Install(void);
extern void	MM_PreinitVirtual(void);
extern void	MM_Install(tMBoot_Info *MBoot);
extern void MM_InstallVirtual(void);
extern void	Threads_Init(void);
extern int	Time_Setup(void);
extern Uint	Proc_Clone(Uint *Err, Uint Flags);
extern void	Threads_Sleep(void);
extern void	Threads_Exit(void);

extern int	Modules_LoadBuiltins(void);

// === GLOBALS ===

// === CODE ===
int kmain(Uint MbMagic, tMBoot_Info *MbInfo)
{
	 int	i;
	tMBoot_Module	*mods;
	
	// Adjust Multiboot structure address
	MbInfo = (void*)( (Uint)MbInfo + KERNEL_BASE );
	
	Desctab_Install();	// Set up GDT and IDT
	MM_PreinitVirtual();	// Initialise vital mappings
	MM_Install( MbInfo );	// Set up physical memory manager
	MM_InstallVirtual();	// Clean up virtual address space
	Heap_Install();		// Create initial heap
	
	Log("Starting Multitasking...");
	// Start Multitasking
	Threads_Init();
	
	// Start Timers
	Time_Setup();
	
	Log("Starting VFS...");
	// Load Virtual Filesystem
	VFS_Init();
	
	// Initialise builtin modules
	Log("Initialising builtin modules...");
	Modules_LoadBuiltins();
	
	Log("Loading %i Modules...", MbInfo->ModuleCount);
	
	// Load initial modules
	mods = (void*)( MbInfo->Modules + KERNEL_BASE );
	for( i = 0; i < MbInfo->ModuleCount; i ++ )
	{
		// Adjust into higher half
		mods[i].Start += KERNEL_BASE;
		mods[i].End += KERNEL_BASE;
		mods[i].String += KERNEL_BASE;
		
		Log("Loading '%s'", mods[i].String);
		
		if( !Module_LoadMem( (void *)mods[i].Start, mods[i].End-mods[i].Start, (char *)mods[i].String ) )
		{
			Log_Warning("ARCH", "Unable to load module\n");
		}
	}
	
	// Pass on to Independent Loader
	Log("Loading Configuration...");
	System_Init( (char*)(MbInfo->CommandLine + KERNEL_BASE) );
	
	// Sleep forever (sleeping beauty)
	for(;;)
		Threads_Sleep();
	return 0;
}
