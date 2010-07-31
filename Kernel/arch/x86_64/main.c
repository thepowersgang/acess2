/*
 * Acess2 x86_64 Project
 */
#include <acess.h>
#include <mboot.h>

// === IMPORTS ===
extern void	Desctab_Init(void);
extern void	MM_InitVirt(void);
extern void	Heap_Install(void);
extern void	Threads_Init(void);
//extern void	Time_Setup(void);
extern void	System_Init(char *Commandline);

extern void	MM_InitPhys_Multiboot(tMBoot_Info *MBoot);

// === PROTOTYPES ===

// === GLOBALS ==
char	*gsBootCmdLine = NULL;

// === CODE ===
void kmain(Uint MbMagic, void *MbInfoPtr)
{
	tMBoot_Info	*mbInfo;
	
	Desctab_Init();
	
	MM_InitVirt();
	*(Uint16*)(0xB8000) = 0x1F00|'C';
	
	switch(MbMagic)
	{
	// Multiboot 1
	case MULTIBOOT_MAGIC:
		// Adjust Multiboot structure address
		mbInfo = (void*)( (Uint)MbInfoPtr + KERNEL_BASE );
		gsBootCmdLine = (char*)( (Uint)mbInfo->CommandLine + KERNEL_BASE);
		Log("gsBootCmdLine = '%s'", gsBootCmdLine);
		
		MM_InitPhys_Multiboot( mbInfo );	// Set up physical memory manager
		break;
	default:
		Panic("Multiboot magic invalid %08x, expected %08x\n",
			MbMagic, MULTIBOOT_MAGIC);
		return ;
	}
	
	Log("gsBootCmdLine = '%s'", gsBootCmdLine);
	
	*(Uint16*)(0xB8000) = 0x1F00|'D';
	Heap_Install();
	
	*(Uint16*)(0xB8000) = 0x1F00|'E';
	Log_Log("Arch", "Starting threading...");
	Threads_Init();
	
	//Time_Setup();
	*(Uint16*)(0xB8000) = 0x1F00|'F';
	
	Log_Log("Arch", "Starting VFS...");
	// Load Virtual Filesystem
	VFS_Init();
	
	*(Uint16*)(0xB8000) = 0x1F00|'Z';
	*(Uint16*)(0xB8002) = 0x1F00|'Z';
	*(Uint16*)(0xB8004) = 0x1F00|'Z';
	*(Uint16*)(0xB8006) = 0x1F00|'Z';
	
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

void StartupPrint(char *String)
{
	
}
