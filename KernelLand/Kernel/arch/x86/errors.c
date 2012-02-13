/*
 * Acess2 - x86 Architecture
 * arch/x86/errors.c
 * - CPU Error Handler
 */
#include <acess.h>
#include <proc.h>
#include <mm_virt.h>

// === CONSTANTS ===
#define	MAX_BACKTRACE	8	//!< Maximum distance to trace the stack backwards

// === IMPORTS ===
extern void	MM_PageFault(Uint Addr, Uint ErrorCode, tRegs *Regs);
extern void	VM8086_GPF(tRegs *Regs);
extern void	Threads_Dump(void);
extern void	Threads_Fault(int Num);
extern int	GetCPUNum(void);
extern void	MM_DumpTables(tVAddr, tVAddr);
extern void	Proc_EnableSSE(void);
extern void	Proc_RestoreSSE(Uint32 Data);

// === PROTOTYPES ===
void	__stack_chk_fail(void);
void	ErrorHandler(tRegs *Regs);
void	Proc_PrintBacktrace(void);
void	Error_Backtrace(Uint eip, Uint ebp);
void	StartupPrint(char *Str);

// === GLOBALS ===
const char *csaERROR_NAMES[] = {
	"Divide By Zero", "Debug", "NMI Exception", "INT3",
	"INTO Instr - Overflow", "BOUND Instr - Out of Bounds", "Invalid Opcode", "Coprocessor not avaliable",
	"Double Fault", "Coprocessor Segment Overrun", "Bad TSS", "Segment Not Present",
	"Stack Fault Exception", "GPF", "#PF", "Reserved",
	"Floating Point Exception", "Alignment Check Exception", "Machine Check Exception",	"Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved"
	};

// === CODE ===
/**
 * \brief Keeps GCC happy
 */
void __stack_chk_fail(void)
{
	Panic("FATAL ERROR: Stack Check Failed\n");
	for(;;);
}

/**
 * \fn void ErrorHandler(tRegs *Regs)
 * \brief General Error Handler
 * \param Regs	Register state at error
 */
void ErrorHandler(tRegs *Regs)
{
	Uint	cr;
	
	//if( Regs && !(Regs->int_num == 13 && Regs->eflags & 0x20000) )
	//	__asm__ __volatile__ ("xchg %bx, %bx");
	//Log_Debug("X86", "Regs = %p", Regs);
	//Log_Debug("X86", "Error %i at 0x%08x", Regs->int_num, Regs->eip);
	
	__asm__ __volatile__ ("cli");
	
	// Debug exception (used for single-stepping)
	if(Regs->int_num == 1)
	{
		static Uint32	lastEIP = 0;
		tThread	*thread = Proc_GetCurThread();
		if( Regs->eip == lastEIP )
			return;
		Log("%p(%i %s) IP=%08x", thread, thread->TID, thread->ThreadName, Regs->eip);
		lastEIP = Regs->eip;
		return ;
	}
	
	// Page Fault
	if(Regs->int_num == 14)
	{
		__asm__ __volatile__ ("mov %%cr2, %0":"=r"(cr));
		MM_PageFault( cr, Regs->err_code, Regs );
		return ;
	}

	// #NM - Coprocessor unavaliable
	if(Regs->int_num == 7)
	{
		tThread	*thread = Proc_GetCurThread();
		if(!thread->SavedState.bSSEModified)
		{
			Proc_EnableSSE();
			if(!thread->SavedState.SSE)
				thread->SavedState.SSE = malloc(sizeof(tSSEState) + 0xF);
			else
				Proc_RestoreSSE( ((Uint)thread->SavedState.SSE + 0xF) & ~0xF );
			thread->SavedState.bSSEModified = 1;
			__asm__ __volatile__ ("sti");
			return ;
		}
		// oops, SSE enabled but a #NM, bad news
	}
	
	// VM8086 GPF
	if(Regs->int_num == 13 && Regs->eflags & 0x20000)
	{
		VM8086_GPF(Regs);
		return ;
	}
	
	// Check if it's a user mode fault
	if( (Regs->cs & 3) == 3 ) {
		Log_Warning("Arch", "User Fault -  %s, Code: 0x%x",
			csaERROR_NAMES[Regs->int_num], Regs->err_code);
		Log_Warning("Arch", "at CS:EIP %04x:%08x",
			Regs->cs, Regs->eip);
		MM_DumpTables(0, KERNEL_BASE);
		switch( Regs->int_num )
		{
		// Division by Zero
		case  0:	Threads_Fault(FAULT_DIV0);	break;
		// Invalid opcode
		case  6:	Threads_Fault(FAULT_OPCODE);	break;
		// GPF
		case 13:	Threads_Fault(FAULT_ACCESS);	break;
		// Floating Point Exception
		case 16:	Threads_Fault(FAULT_FLOAT);	break;
		
		default:	Threads_Fault(FAULT_MISC);	break;
		}
		return ;
	}
	
	Debug_KernelPanic();
	
	LogF("CPU %i Error %i - %s, Code: 0x%x - At %08x\n",
		GetCPUNum(),
		Regs->int_num, csaERROR_NAMES[Regs->int_num], Regs->err_code,
		Regs->eip);
	
	//Warning("CPU Error %i - %s, Code: 0x%x",
	//	Regs->int_num, csaERROR_NAMES[Regs->int_num], Regs->err_code);
	//Warning(" CS:EIP = 0x%04x:%08x", Regs->cs, Regs->eip);
	__ASM__ ("xchg %bx, %bx");
	if(Regs->cs == 0x08)
		Warning(" SS:ESP = 0x0010:%08x", (Uint)Regs+sizeof(tRegs));
	else
		Warning(" SS:ESP = 0x%04x:%08x", Regs->ss, Regs->esp);
	Warning(" EFLAGS = 0x%08x", Regs->eflags);
	Warning(" EAX %08x ECX %08x EDX %08x EBX %08x",
		Regs->eax, Regs->ecx, Regs->edx, Regs->ebx);
	Warning(" ESP %08x EBP %08x ESI %08x EDI %08x",
		Regs->esp, Regs->ebp, Regs->esi, Regs->edi);
	Warning(" DS %04x ES %04x FS %04x GS %04x",
		Regs->ds, Regs->es, Regs->fs, Regs->gs);
	
	// Control Registers
	__asm__ __volatile__ ("mov %%cr0, %0":"=r"(cr));
	Warning(" CR0 0x%08x", cr);
	__asm__ __volatile__ ("mov %%cr2, %0":"=r"(cr));
	Warning(" CR2 0x%08x", cr);
	__asm__ __volatile__ ("mov %%cr3, %0":"=r"(cr));
	Warning(" CR3 0x%08x", cr);
	
	switch( Regs->int_num )
	{
	case 6:	// #UD
		Warning(" Offending bytes: %02x %02x %02x %02x",
			*(Uint8*)(Regs->eip+0), *(Uint8*)(Regs->eip+1),
			*(Uint8*)(Regs->eip+2), *(Uint8*)(Regs->eip+3));
		break;
	}
	
	// Print Stack Backtrace
	Error_Backtrace(Regs->eip, Regs->ebp);
	
	// Dump running threads
	Threads_Dump();
	
	for(;;)	__asm__ __volatile__ ("hlt");
}

void Proc_PrintBacktrace(void)
{
	Uint32	ebp;
	__asm__ __volatile__ ("mov %%ebp, %0" : "=r" (ebp));
	Error_Backtrace( *(Uint32*)(ebp+4), *(Uint32*)ebp );
}

/**
 * \fn void Error_Backtrace(Uint eip, Uint ebp)
 * \brief Unrolls the stack to trace execution
 * \param eip	Current Instruction Pointer
 * \param ebp	Current Base Pointer (Stack Frame)
 */
void Error_Backtrace(Uint eip, Uint ebp)
{
	 int	i = 0;
//	Uint	delta = 0;
//	char	*str = NULL;
	
	//if(eip < 0xC0000000 && eip > 0x1000)
	//{
	//	LogF("Backtrace: User - 0x%x\n", eip);
	//	return;
	//}

	#if 0	
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
	#endif	

	//str = Debug_GetSymbol(eip, &delta);
//	if(str == NULL)
		LogF("Backtrace: 0x%x", eip);
//	else
//		LogF("Backtrace: %s+0x%x", str, delta);
	if(!MM_GetPhysAddr(ebp))
	{
		LogF("\nBacktrace: Invalid EBP, stopping\n");
		return;
	}
	
	
	while( MM_GetPhysAddr(ebp) && i < MAX_BACKTRACE )
	{
		if( ebp >= MM_KERNEL_STACKS_END )	break;
		//str = Debug_GetSymbol(*(Uint*)(ebp+4), &delta);
//		if(str == NULL)
			LogF(" >> 0x%x", *(Uint*)(ebp+4));
//		else
//			LogF(" >> %s+0x%x", str, delta);
		ebp = *(Uint*)ebp;
		i++;
	}
	LogF("\n");
}

/**
 * \fn void StartupPrint(char *Str)
 * \brief Str	String to print
 * \note WHY IS THIS HERE?!?!
 */
void StartupPrint(char *Str)
{
	Uint16	*buf = (void*)0xC00B8000;
	 int	i = 0;
	static int	line = 0;
	while(*Str)
	{
		buf[line*80 + i++] = *Str | 0x0700;
		Str ++;
	}
	
	// Clear the rest of the line
	while(i < 80)
		buf[line*80 + i++] = 0x0720;
	
	line ++;
	if(line == 25)
	{
		line --;
		memcpy(buf, &buf[80], 80*24*2);
		memset(&buf[80*24], 0, 80*2);
	}
}

// === EXPORTS ===
EXPORT(__stack_chk_fail);
