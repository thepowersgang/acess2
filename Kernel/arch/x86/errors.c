/*
 * Acess2 - x86 Architecture
 * arch/x86/errors.c
 * - CPU Error Handler
 */
#include <common.h>
#include <proc.h>

// === CONSTANTS ===
#define	MAX_BACKTRACE	8	//!< Maximum distance to trace the stack backwards

// === IMPORTS ===
extern void	MM_PageFault(Uint Addr, Uint ErrorCode, tRegs *Regs);
extern void Threads_Dump();

// === PROTOTYPES ===
void	Error_Backtrace(Uint eip, Uint ebp);

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
	
	// Print Stack Backtrace
	Error_Backtrace(Regs->eip, Regs->ebp);
	
	// Dump running threads
	Threads_Dump();
	
	for(;;)	__asm__ __volatile__ ("hlt");
}
/**
 * \fn void Error_Backtrace(Uint eip, Uint ebp)
 * \brief Unrolls the stack to trace execution
 */
void Error_Backtrace(Uint eip, Uint ebp)
{
	 int	i = 0;
	Uint	delta = 0;
	char	*str = NULL;
	
	//if(eip < 0xC0000000 && eip > 0x1000)
	//{
	//	LogF("Backtrace: User - 0x%x\n", eip);
	//	return;
	//}
	
	if(eip > 0xE0000000)
	{
		LogF("Backtrace: Data Area - 0x%x\n", eip);
		return;
	}
	
	if(eip > 0xC8000000)
	{
		LogF("Backtrace: Kernel Module - 0x%x\n", eip);
		return;
	}
	
	//str = Debug_GetSymbol(eip, &delta);
	if(str == NULL)
		LogF("Backtrace: 0x%x", eip);
	else
		LogF("Backtrace: %s+0x%x", str, delta);
	if(!MM_GetPhysAddr(ebp))
	{
		LogF("\nBacktrace: Invalid EBP, stopping\n");
		return;
	}
	
	
	while( MM_GetPhysAddr(ebp) && i < MAX_BACKTRACE )
	{
		//str = Debug_GetSymbol(*(Uint*)(ebp+4), &delta);
		if(str == NULL)
			LogF(" >> 0x%x", *(Uint*)(ebp+4));
		else
			LogF(" >> %s+0x%x", str, delta);
		ebp = *(Uint*)ebp;
		i++;
	}
	LogF("\n");
}
