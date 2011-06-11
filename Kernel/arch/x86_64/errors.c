/*
 * Acess2 x86_64 Project
 * - Error Handling
 */
#include <acess.h>
#include <proc.h>

// === IMPORTS ===
void	MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs);

// === PROTOTYPES ===
void	Error_Handler(tRegs *Regs);

// === GLOBALS ==
const char * const csaERROR_NAMES[] = {
	"Divide By Zero", "Debug", "NMI Exception", "INT3",
	"INTO", "Out of Bounds", "Invalid Opcode", "Coprocessor not avaliable",
	"Double Fault", "Coprocessor Segment Overrun", "Bad TSS", "Segment Not Present",
	"Stack Fault Exception", "GPF", "#PF", "Reserved",
	"Floating Point Exception", "Alignment Check Exception", "Machine Check Exception",	"Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved"
	};

// === CODE ===
void Error_Handler(tRegs *Regs)
{
	Uint	cr;
	
	if( Regs->IntNum == 14 ) {
		__asm__ __volatile__ ("mov %%cr2, %0":"=r"(cr));
		MM_PageFault(cr, Regs->ErrorCode, Regs);
		return ;
	}
	
	Debug_KernelPanic();
	
	Log("CPU Error %x, Code: 0x%x", Regs->IntNum, Regs->ErrorCode);
//	Log(" - %s", csaERROR_NAMES[Regs->IntNum]);
	Log(" CS:RIP = 0x%04x:%016llx", Regs->CS, Regs->RIP);
	Log(" SS:RSP = 0x%04x:%016llx", Regs->SS, Regs->RSP);
	Log(" RFLAGS = 0x%016llx", Regs->RFlags);
	
	Log(" RAX %016llx RCX %016llx RDX %016llx RBX %016llx",
		Regs->RAX, Regs->RCX, Regs->RDX, Regs->RBX);
	Log(" RSP %016llx RBP %016llx RSI %016llx RDI %016llx",
		Regs->RSP, Regs->RBP, Regs->RSP, Regs->RDI);
	Log(" R8  %016llx R9  %016llx R10 %016llx R11 %016llx",
		Regs->R8, Regs->R9, Regs->R10, Regs->R11);
	Log(" R12 %016llx R13 %016llx R14 %016llx R15 %016llx",
		Regs->R12, Regs->R13, Regs->R14, Regs->R15);
	Log(" FS %04x GS %04x", Regs->FS, Regs->GS);
	
	
	// Control Registers
	__asm__ __volatile__ ("mov %%cr0, %0":"=r"(cr));
	Warning(" CR0 0x%08x", cr);
	__asm__ __volatile__ ("mov %%cr2, %0":"=r"(cr));
	Warning(" CR2 0x%016llx", cr);
	__asm__ __volatile__ ("mov %%cr3, %0":"=r"(cr));
	Warning(" CR3 0x%016llx", cr);
	__asm__ __volatile__ ("mov %%cr4, %0":"=r"(cr));
	Warning(" CR4 0x%08x", cr);
	
	switch( Regs->IntNum )
	{
	case 6:	// #UD
		Warning(" Offending bytes: %02x %02x %02x %02x",
			*(Uint8*)(Regs->RIP+0), *(Uint8*)(Regs->RIP+1),
			*(Uint8*)(Regs->RIP+2), *(Uint8*)(Regs->RIP+3)
			);
		break;
	}
	
	__asm__ __volatile__ ("cli");
	for(;;)
		__asm__ __volatile__ ("hlt");
}
