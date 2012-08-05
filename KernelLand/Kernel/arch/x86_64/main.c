/*
 * Acess2 x86_64 Project
 */
#include <acess.h>
#include <mboot.h>
#include <init.h>

// === CONSTANTS ===
#define MAX_PMEMMAP_ENTS	16

// === IMPORTS ===
extern void	Desctab_Init(void);
extern void	MM_InitVirt(void);
extern void	Heap_Install(void);
extern int	Time_Setup(void);

extern void	MM_InitPhys_Multiboot(tMBoot_Info *MBoot);

// === PROTOTYPES ===
void	kmain(Uint MbMagic, void *MbInfoPtr);

// === GLOBALS ==
char	*gsBootCmdLine = NULL;

// === CODE ===
void kmain(Uint MbMagic, void *MbInfoPtr)
{
	tMBoot_Info	*mbInfo;

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
		MM_InitPhys_Multiboot( mbInfo );	// Set up physical memory manager
		break;
	default:
		Panic("Multiboot magic invalid %08x, expected %08x\n",
			MbMagic, MULTIBOOT_MAGIC);
		return ;
	}
	
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
