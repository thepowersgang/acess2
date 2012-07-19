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
#include <pmemmap.h>

#define	VGA_ERRORS	0

#define KERNEL_LOAD	0x100000	// 1MiB
#define MAX_ARGSTR_POS	(0x400000-0x2000)
#define MAX_PMEMMAP_ENTS	16

// === IMPORTS ===
extern char	gKernelEnd[];
extern void	Heap_Install(void);
extern void	MM_PreinitVirtual(void);
extern void	MM_Install(int NPMemRanges, tPMemMapEnt *PMemRanges);
extern void	MM_InstallVirtual(void);
extern int	Time_Setup(void);

// === PROTOTYPES ===
 int	kmain(Uint MbMagic, void *MbInfoPtr);

// === GLOBALS ===
char	*gsBootCmdLine = NULL;
struct {
	Uint32	PBase;
	void	*Base;
	Uint	Size;
	char	*ArgString;
}	*gaArch_BootModules;
 int	giArch_NumBootModules = 0;

// === CODE ===
int kmain(Uint MbMagic, void *MbInfoPtr)
{
	tMBoot_Module	*mods;
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
		
		tMBoot_MMapEnt	*ent = (void*)mbInfo->MMapAddr;
		tMBoot_MMapEnt	*last = (void*)(mbInfo->MMapAddr + mbInfo->MMapLength);
		
		// Build up memory map
		nPMemMapEnts = 0;
		while( ent < last && nPMemMapEnts < MAX_PMEMMAP_ENTS )
		{
			tPMemMapEnt	*nent = &pmemmap[nPMemMapEnts];
			nent->Start = ent->Base;
			nent->Length = ent->Length;
			switch(ent->Type)
			{
			case 1:
				nent->Type = PMEMTYPE_FREE;
				break;
			default:
				nent->Type = PMEMTYPE_RESERVED;
				break;
			}
			nent->NUMADomain = 0;
			
			nPMemMapEnts ++;
			ent = (void*)( (tVAddr)ent + ent->Size + 4 );
		}

		// Ensure it's valid
		nPMemMapEnts = PMemMap_ValidateMap(pmemmap, nPMemMapEnts, MAX_PMEMMAP_ENTS);
		// TODO: Error handling

		// Replace kernel with PMEMTYPE_USED
		nPMemMapEnts = PMemMap_MarkRangeUsed(
			pmemmap, nPMemMapEnts, MAX_PMEMMAP_ENTS,
			KERNEL_LOAD, (tVAddr)&gKernelEnd - KERNEL_LOAD - KERNEL_BASE
			);

		// Replace modules with PMEMTYPE_USED
		nPMemMapEnts = PMemMap_MarkRangeUsed(pmemmap, nPMemMapEnts, MAX_PMEMMAP_ENTS,
			mbInfo->Modules, mbInfo->ModuleCount*sizeof(*mods)
			);
		mods = (void*)mbInfo->Modules;
		for( int i = 0; i < mbInfo->ModuleCount; i ++ )
		{
			nPMemMapEnts = PMemMap_MarkRangeUsed(
				pmemmap, nPMemMapEnts, MAX_PMEMMAP_ENTS,
				mods->Start, mods->End - mods->Start
				);
		}
		
		// Debug - Output map
		PMemMap_DumpBlocks(pmemmap, nPMemMapEnts);

		// Adjust Multiboot structure address
		mbInfo = (void*)( (Uint)MbInfoPtr + KERNEL_BASE );
		
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
	for( int i = 0; i < mbInfo->ModuleCount; i ++ )
	{
		 int	ofs;
	
		Log_Log("Arch", "Multiboot Module at 0x%08x, 0x%08x bytes (String at 0x%08x)",
			mods[i].Start, mods[i].End-mods[i].Start, mods[i].String);
	
		gaArch_BootModules[i].PBase = mods[i].Start;
		gaArch_BootModules[i].Size = mods[i].End - mods[i].Start;
	
		// Always HW map the module data	
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
	 int	i, j, numPages;
	for( i = 0; i < giArch_NumBootModules; i ++ )
	{
		Log_Log("Arch", "Loading '%s'", gaArch_BootModules[i].ArgString);
		
		if( !Module_LoadMem( gaArch_BootModules[i].Base, gaArch_BootModules[i].Size, gaArch_BootModules[i].ArgString ) )
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
	free( gaArch_BootModules );
}
