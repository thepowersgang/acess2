/*
 * Acess2 - x86 Architecture
 * arch/x86/errors.c
 * - CPU Error Handler
 */
#include <common.h>

// === IMPORTS ===
extern void	MM_PageFault(Uint Addr, Uint ErrorCode, tRegs *Regs);
extern void Threads_Dump();

// === CODE ===
/**
 * \fn void ErrorHandler(tRegs *Regs)
 * \brief General Error Handler
 */
void ErrorHandler(tRegs *Regs)
{
	Uint	cr;
	__asm__ __volatile__ ("cli");
	
	if(Regs->int_num == 14)
	{
		__asm__ __volatile__ ("mov %%cr2, %0":"=r"(cr));
		MM_PageFault( cr, Regs->err_code, Regs );
		return ;
	}
	
	Warning("CPU Error %i, Code: 0x%x", Regs->int_num, Regs->err_code);
	Warning(" CS:EIP = 0x%04x:%08x", Regs->cs, Regs->eip);
	Warning(" SS:ESP = 0x%04x:%08x", Regs->ss, Regs->esp);
	Warning(" EFLAGS = 0x%08x", Regs->eflags);
	Warning(" EAX %08x EBX %08x", Regs->eax, Regs->ebx);
	Warning(" ECX %08x EDX %08x", Regs->ecx, Regs->edx);
	Warning(" ESP %08x EBP %08x", Regs->esp, Regs->ebp);
	Warning(" ESI %08x EDI %08x", Regs->esi, Regs->edi);
	Warning(" DS %04x ES %04x", Regs->ds, Regs->es);
	Warning(" FS %04x GS %04x", Regs->fs, Regs->gs);
	
	// Control Registers
	__asm__ __volatile__ ("mov %%cr0, %0":"=r"(cr));
	Warning(" CR0: 0x%08x", cr);
	__asm__ __volatile__ ("mov %%cr2, %0":"=r"(cr));
	Warning(" CR2: 0x%08x", cr);
	__asm__ __volatile__ ("mov %%cr3, %0":"=r"(cr));
	Warning(" CR3: 0x%08x", cr);
	
	// Dump running threads
	Threads_Dump();
	
	for(;;)	__asm__ __volatile__ ("hlt");
}
