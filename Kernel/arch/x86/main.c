/*
 * Acess 2
 * x86 Kernel Main
 * arch/x86/main.c
 */
#include <acess.h>
#include <mboot.h>
#include <multiboot2.h>
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
char	*gsBootCmdLine = NULL;

// === CODE ===
int kmain(Uint MbMagic, void *MbInfoPtr)
{
	 int	i;
	tMBoot_Module	*mods;
	tMBoot_Info	*mbInfo;
	
	Log("MbMagic = %08x", MbMagic);
	Log("MbInfoPtr = %p", MbInfoPtr);
	
	// Set up non-boot info dependent stuff
	Desctab_Install();	// Set up GDT and IDT
	MM_PreinitVirtual();	// Initialise vital mappings
	
	switch(MbMagic)
	{
	// Multiboot 1
	case MULTIBOOT_MAGIC:
		// Adjust Multiboot structure address
		mbInfo = (void*)( (Uint)MbInfoPtr + KERNEL_BASE );
		gsBootCmdLine = (char*)(mbInfo->CommandLine + KERNEL_BASE);
		
		MM_Install( mbInfo );	// Set up physical memory manager
		break;
	
	// Multiboot 2
	case MULTIBOOT2_MAGIC:
		Warning("Multiboot 2 Not yet supported");
		//MM_InstallMBoot2( MbInfo );	// Set up physical memory manager
		return 0;
		break;
	
	default:
		Panic("Multiboot magic invalid %08x, expected %08x or %08x\n",
			MbMagic, MULTIBOOT_MAGIC, MULTIBOOT2_MAGIC);
		return 0;
	}
	
	MM_InstallVirtual();	// Clean up virtual address space
	Heap_Install();		// Create initial heap
	
	//Log_Log("Arch", "Starting Multitasking...");
	// Start Multitasking
	Threads_Init();
	
	// Start Timers
	Time_Setup();
	
	Log_Log("Arch", "Starting VFS...");
	// Load Virtual Filesystem
	VFS_Init();
	
	// Initialise builtin modules
	Log_Log("Arch", "Initialising builtin modules...");
	Modules_LoadBuiltins();
	
	Log_Log("Arch", "Loading %i Modules...", mbInfo->ModuleCount);
	
	// Load initial modules
	mods = (void*)( mbInfo->Modules + KERNEL_BASE );
	for( i = 0; i < mbInfo->ModuleCount; i ++ )
	{
		// Adjust into higher half
		mods[i].Start += KERNEL_BASE;
		mods[i].End += KERNEL_BASE;
		mods[i].String += KERNEL_BASE;
		
		Log_Log("Arch", "Loading '%s'", mods[i].String);
		
		if( !Module_LoadMem( (void *)mods[i].Start, mods[i].End-mods[i].Start, (char *)mods[i].String ) )
		{
			Log_Warning("Arch", "Unable to load module\n");
		}
	}
	
	// Pass on to Independent Loader
	Log_Log("Arch", "Starting system");
	System_Init( gsBootCmdLine );
	
	// Sleep forever (sleeping beauty)
	for(;;)
		Threads_Sleep();
	return 0;
}
