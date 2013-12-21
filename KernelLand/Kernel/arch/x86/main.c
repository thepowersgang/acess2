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
// - Modules/Display/VESA
extern void	VBE_int_SetBootMode(Uint16 ModeID, const void *ModeInfo);

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
		ASSERT( mbInfo->CommandLine < 4*1024*1024 );
		gsBootCmdLine = (char*)(mbInfo->CommandLine + KERNEL_BASE);

		// Adjust Multiboot structure address
		mbInfo = (void*)( (tVAddr)MbInfoPtr + KERNEL_BASE );

		// Parse memory map
		// - mbInfo->Flags is checked in this function
		nPMemMapEnts = Multiboot_LoadMemoryMap(mbInfo, KERNEL_BASE, pmemmap, MAX_PMEMMAP_ENTS,
			KERNEL_LOAD, (tVAddr)&gKernelEnd - KERNEL_BASE);
		
		
		// Get video mode
		Debug("mbInfo->Flags = 0x%x", mbInfo->Flags);
		if( mbInfo->Flags & (1 << 11) )
		{
			// TODO: Ensure address is in mapped area
			ASSERT( mbInfo->vbe_mode_info < 4*1024*1024 );
			VBE_int_SetBootMode(mbInfo->vbe_mode, (void*)(mbInfo->vbe_mode_info + KERNEL_BASE));
		}

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
	for( int i = 0; i < giArch_NumBootModules; i ++ )
	{
		const tBootModule	*mod = &gaArch_BootModules[i];
		Log_Log("Arch", "Loading (%p[%P]+%x) '%s'",
			mod->Base, mod->PBase, mod->Size,
			mod->ArgString);
		
		if( !Module_LoadMem( mod->Base, mod->Size, mod->ArgString) ) {
			Log_Warning("Arch", "Unable to load module");
			continue ;
		}
		
		#if 0
		// Unmap and free
		int numPages = (mod->Size + ((tVAddr)mod->Base&0xFFF) + 0xFFF) >> 12;
		MM_UnmapHWPages( (tVAddr)gaArch_BootModules[i].Base, numPages );
		
		//for( int j = 0; j < numPages; j++ )
		//	MM_DerefPhys( mod->PBase + (j << 12) );
		
		if( (tVAddr) mod->ArgString > MAX_ARGSTR_POS )
			MM_UnmapHWPages( (tVAddr)mod->ArgString, 2 );
		#endif
	}
	Log_Log("Arch", "Boot modules loaded");
	if( gaArch_BootModules )
		free( gaArch_BootModules );
}
