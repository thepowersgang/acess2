/*
 * Acess 2
 * x86 Kernel Main
 * arch/x86/main.c
 */
#include <common.h>
#include <mboot.h>
#include <init.h>
#include <mm_virt.h>
#include <mp.h>

#define	VGA_ERRORS	0

// === IMPORTS ===
extern void	Heap_Install();
extern void	Desctab_Install();
extern void	MM_PreinitVirtual();
extern void	MM_Install(tMBoot_Info *MBoot);
extern void MM_InstallVirtual();
extern void	Threads_Init();
extern int	Time_Setup();
extern Uint	Proc_Clone(Uint *Err, Uint Flags);
extern void	Threads_Sleep();
extern void	Threads_Exit();

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
	
	Log("Loading Modules...");
	
	// Load initial modules
	mods = (void*)( MbInfo->Modules + KERNEL_BASE );
	for(i=0;i<MbInfo->ModuleCount;i++)
	{
		// Adjust into higher half
		mods[i].Start += KERNEL_BASE;
		mods[i].End += KERNEL_BASE;
		mods[i].String += KERNEL_BASE;
		
		Log("Loading '%s'", mods[i].String);
		
		if( !Module_LoadMem( (void *)mods[i].Start, mods[i].End-mods[i].Start, (char *)mods[i].String ) )
		{
			Warning("Unable to load module\n");
		}
	}
	
	// Pass on to Independent Loader
	Log("Loading Configuration...");
	System_Init( (char*)(MbInfo->CommandLine + KERNEL_BASE) );
	
	// Sleep forever (sleeping beauty)
	for(;;)	Threads_Sleep();
	return 0;
}
