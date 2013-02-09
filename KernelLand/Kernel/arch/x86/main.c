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
#include <pmemmap.h>

#define	VGA_ERRORS	0

#define KERNEL_LOAD	0x100000	// 1MiB
#define MAX_ARGSTR_POS	(0x400000-0x2000)
#define MAX_PMEMMAP_ENTS	16

// === IMPORTS ===
extern char	gKernelEnd[];
extern void	MM_PreinitVirtual(void);
extern void	MM_Install(int NPMemRanges, tPMemMapEnt *PMemRanges);
extern void	MM_InstallVirtual(void);
extern int	Time_Setup(void);
extern int	ACPICA_Initialise(void);

// === PROTOTYPES ===
 int	kmain(Uint MbMagic, void *MbInfoPtr);

// === GLOBALS ===
char	*gsBootCmdLine = NULL;
tBootModule	*gaArch_BootModules;
 int	giArch_NumBootModules = 0;

// === CODE ===
int kmain(Uint MbMagic, void *MbInfoPtr)
{
	tMBoot_Info	*mbInfo;
	tPMemMapEnt	pmemmap[MAX_PMEMMAP_ENTS];
	 int	nPMemMapEnts;

	LogF("%s\r\n", gsBuildInfo);
	
	MM_PreinitVirtual();	// Initialise virtual mappings

	mbInfo = MbInfoPtr;	

	switch(MbMagic)
	{
	// Multiboot 1
	case MULTIBOOT_MAGIC: {
		// TODO: Handle when this isn't in the mapped area
		gsBootCmdLine = (char*)(mbInfo->CommandLine + KERNEL_BASE);

		// Adjust Multiboot structure address
		mbInfo = (void*)( (tVAddr)MbInfoPtr + KERNEL_BASE );

		nPMemMapEnts = Multiboot_LoadMemoryMap(mbInfo, KERNEL_BASE, pmemmap, MAX_PMEMMAP_ENTS,
			KERNEL_LOAD, (tVAddr)&gKernelEnd - KERNEL_BASE);

		break; }
	
	// Multiboot 2
	case MULTIBOOT2_MAGIC:
		Panic("Multiboot 2 not yet supported");
		//MM_InstallMBoot2( MbInfo );	// Set up physical memory manager
		return 0;
		break;
	
	default:
		Panic("Multiboot magic invalid %08x, expected %08x or %08x\n",
			MbMagic, MULTIBOOT_MAGIC, MULTIBOOT2_MAGIC);
		return 0;
	}
	
	// Set up physical memory manager
	MM_Install(nPMemMapEnts, pmemmap);
	
	MM_InstallVirtual();	// Clean up virtual address space
	Heap_Install();		// Create initial heap
	Time_Setup();	// Initialise timing
	
	// Start Multitasking
	Threads_Init();

	#if USE_ACPICA
	// Poke ACPICA
	ACPICA_Initialise();
	#endif

	Log_Log("Arch", "Starting VFS...");
	// Load Virtual Filesystem
	VFS_Init();
	
	// Load initial modules
	gaArch_BootModules = Multiboot_LoadModules(mbInfo, KERNEL_BASE, &giArch_NumBootModules);
	
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
	 int	i, j, numPages;
	for( i = 0; i < giArch_NumBootModules; i ++ )
	{
		Log_Log("Arch", "Loading '%s'", gaArch_BootModules[i].ArgString);
		
		if( !Module_LoadMem( gaArch_BootModules[i].Base,
			gaArch_BootModules[i].Size, gaArch_BootModules[i].ArgString
			) )
		{
			Log_Warning("Arch", "Unable to load module");
		}
		
		// Unmap and free
		numPages = (gaArch_BootModules[i].Size + ((Uint)gaArch_BootModules[i].Base&0xFFF) + 0xFFF) >> 12;
		MM_UnmapHWPages( (tVAddr)gaArch_BootModules[i].Base, numPages );
		
		for( j = 0; j < numPages; j++ )
			MM_DerefPhys( gaArch_BootModules[i].PBase + (j << 12) );
		
		if( (tVAddr) gaArch_BootModules[i].ArgString > MAX_ARGSTR_POS )
			MM_UnmapHWPages( (tVAddr)gaArch_BootModules[i].ArgString, 2 );
	}
	Log_Log("Arch", "Boot modules loaded");
	if( gaArch_BootModules )
		free( gaArch_BootModules );
}
