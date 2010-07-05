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

#define MAX_ARGSTR_POS	(0x400000-0x2000)

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
// --- Core ---
extern void	System_Init(char *Commandline);

// === PROTOTYPES ===
void	Arch_LoadBootModules(void);

// === GLOBALS ===
char	*gsBootCmdLine = NULL;
struct {
	void	*Base;
	Uint	Size;
	char	*ArgString;
}	*gaArch_BootModules;
 int	giArch_NumBootModules = 0;

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
	MM_PreinitVirtual();	// Initialise virtual mappings
	
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
	
	// Load initial modules
	mods = (void*)( mbInfo->Modules + KERNEL_BASE );
	giArch_NumBootModules = mbInfo->ModuleCount;
	gaArch_BootModules = malloc( giArch_NumBootModules * sizeof(*gaArch_BootModules) );
	for( i = 0; i < mbInfo->ModuleCount; i ++ )
	{
		 int	ofs;
	
		Log_Log("Arch", "Multiboot Module at 0x%08x, 0x%08x bytes (String at 0x%08x)",
			mods[i].Start, mods[i].End-mods[i].Start, mods[i].String);
	
		// Always HW map the module data
		gaArch_BootModules[i].Size = mods[i].End - mods[i].Start;
		
		ofs = mods[i].Start&0xFFF;
		gaArch_BootModules[i].Base = (void*)( MM_MapHWPages(mods[i].Start,
			(gaArch_BootModules[i].Size+ofs+0xFFF) / 0x1000
			) + ofs );
		
		// Only map the string if needed
		if( (tVAddr)mods[i].String > MAX_ARGSTR_POS )
		{
			// Assumes the string is < 4096 bytes long)
			gaArch_BootModules[i].ArgString = (void*)(
				MM_MapHWPages(mods[i].String, 2) + (mods[i].String&0xFFF)
				);
		}
		else
			gaArch_BootModules[i].ArgString = (char *)mods[i].String + KERNEL_BASE;
	}
	
	// Pass on to Independent Loader
	Log_Log("Arch", "Starting system");
	System_Init(gsBootCmdLine);
	
	// Sleep forever (sleeping beauty)
	for(;;)
		Threads_Sleep();
	return 0;
}

void Arch_LoadBootModules(void)
{
	 int	i;
	for( i = 0; i < giArch_NumBootModules; i ++ )
	{
		Log_Debug("Arch", "Module %i: %p - %p 0x%x",
			i, gaArch_BootModules[i].ArgString,
			gaArch_BootModules[i].Base, gaArch_BootModules[i].Size
			);
		Log_Log("Arch", "Loading '%s'", gaArch_BootModules[i].ArgString);
		
		if( !Module_LoadMem( gaArch_BootModules[i].Base, gaArch_BootModules[i].Size, gaArch_BootModules[i].ArgString ) )
		{
			Log_Warning("Arch", "Unable to load module");
		}
		
		MM_UnmapHWPages(
			(tVAddr)gaArch_BootModules[i].Base,
			(gaArch_BootModules[i].Size + ((Uint)gaArch_BootModules[i].Base&0xFFF) + 0xFFF) >> 12
			);
		
		if( (tVAddr) gaArch_BootModules[i].ArgString > MAX_ARGSTR_POS )
			MM_UnmapHWPages( (tVAddr)gaArch_BootModules[i].ArgString, 2 );
	}
	Log_Log("Arch", "Boot modules loaded");
	free( gaArch_BootModules );
}
