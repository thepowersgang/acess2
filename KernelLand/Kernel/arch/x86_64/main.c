/*
 * Acess2 Kernel x86_64
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Kernel C entrypoint
 */
#include <acess.h>
#include <mboot.h>
#include <init.h>
#include <archinit.h>
#include <pmemmap.h>

// === CONSTANTS ===
#define KERNEL_LOAD	0x100000
#define MAX_PMEMMAP_ENTS	16

// === IMPORTS ===
extern void	Desctab_Init(void);
extern void	MM_InitVirt(void);
extern void	Heap_Install(void);
extern int	Time_Setup(void);

extern char	gKernelEnd[];

// === PROTOTYPES ===
void	kmain(Uint MbMagic, void *MbInfoPtr);

// === GLOBALS ==
char	*gsBootCmdLine = NULL;

// === CODE ===
void kmain(Uint MbMagic, void *MbInfoPtr)
{
	tMBoot_Info	*mbInfo;
	tPMemMapEnt	pmemmap[MAX_PMEMMAP_ENTS];
	 int	nPMemMapEnts;

	LogF("%s\r\n", gsBuildInfo);
	
	Desctab_Init();

	MM_InitVirt();
	*(Uint16*)(KERNEL_BASE|0xB8000) = 0x1F00|'C';
	
	switch(MbMagic)
	{
	// Multiboot 1
	case MULTIBOOT_MAGIC:
		// Adjust Multiboot structure address
		mbInfo = (void*)( (Uint)MbInfoPtr + KERNEL_BASE );
		gsBootCmdLine = (char*)( (Uint)mbInfo->CommandLine + KERNEL_BASE);
		nPMemMapEnts = Multiboot_LoadMemoryMap(mbInfo, KERNEL_BASE, pmemmap, MAX_PMEMMAP_ENTS,
			KERNEL_LOAD, (tVAddr)&gKernelEnd - KERNEL_BASE
			);
		break;
	default:
		Panic("Multiboot magic invalid %08x, expected %08x\n",
			MbMagic, MULTIBOOT_MAGIC);
		return ;
	}
	
	MM_InitPhys( nPMemMapEnts, pmemmap );	// Set up physical memory manager
	Log("gsBootCmdLine = '%s'", gsBootCmdLine);
	
	*(Uint16*)(KERNEL_BASE|0xB8000) = 0x1F00|'D';
	Heap_Install();
	
	*(Uint16*)(KERNEL_BASE|0xB8000) = 0x1F00|'E';
	Threads_Init();
	
	Time_Setup();
	*(Uint16*)(KERNEL_BASE|0xB8000) = 0x1F00|'F';
	
	// Load Virtual Filesystem
	Log_Log("Arch", "Starting VFS...");
	VFS_Init();
	
	*(Uint16*)(KERNEL_BASE|0xB8000) = 0x1F00|'Z';
	
	// Pass on to Independent Loader
	Log_Log("Arch", "Starting system");
	System_Init(gsBootCmdLine);
	
	// Sleep forever (sleeping beauty)
	for(;;)
		Threads_Sleep();
}

void Arch_LoadBootModules(void)
{
	
}

void StartupPrint(const char *String)
{
	
}
