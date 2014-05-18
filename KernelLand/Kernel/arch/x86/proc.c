/*
 * Acess2 Kernel (x86)
 * - By John Hodge (thePowersGang)
 *
 * proc.c
 * - Low level thread management
 */
#define DEBUG	0
#include <acess.h>
#include <threads.h>
#include <proc.h>
#include <desctab.h>
#include <mm_virt.h>
#include <errno.h>
#include <hal_proc.h>
#include <arch_int.h>
#include <proc_int.h>
#if USE_MP
# include <apic.h>
#endif

// === FLAGS ===
#define DEBUG_TRACE_SWITCH	0
#define DEBUG_DISABLE_DOUBLEFAULT	1
#define DEBUG_VERY_SLOW_PERIOD	0
#define DEBUG_NOPREEMPT	1
#define DISABLE_PIT	0

// === CONSTANTS ===
// Base is 1193182
#define TIMER_BASE      1193182
#if DISABLE_PIT
# define TIMER_DIVISOR	0xFFFF
#elif DEBUG_VERY_SLOW_PERIOD
# define TIMER_DIVISOR	1193	//~10Hz switch, with 10 quantum = 1s per thread
#else
# define TIMER_DIVISOR	11932	//~100Hz
#endif

// === TYPES ===

// === IMPORTS ===
extern tGDT	gGDT[];
extern tIDT	gIDT[];
extern void	APWait(void);	// 16-bit AP pause code
extern void	APStartup(void);	// 16-bit AP startup code
extern void	NewTaskHeader(tThread *Thread, void *Fcn, int nArgs, ...);	// Actually takes cdecl args
extern Uint	Proc_CloneInt(Uint *ESP, Uint32 *CR3, int bNoUserClone);
extern Uint32	gaInitPageDir[1024];	// start.asm
extern char	Kernel_Stack_Top[];
extern int	giNumCPUs;
extern tThread	gThreadZero;
extern tProcess	gProcessZero;
extern void	Isr8(void);	// Double Fault
extern void	Proc_ReturnToUser(tVAddr Handler, Uint Argument, tVAddr KernelStack);
extern char	scheduler_return[];	// Return address in SchedulerBase
extern char	IRQCommon[];	// Common IRQ handler code
extern char	IRQCommon_handled[];	// IRQCommon call return location
extern char	GetEIP_Sched_ret[];	// GetEIP call return location
extern void	Timer_CallTimers(void);

// === PROTOTYPES ===
//void	ArchThreads_Init(void);
#if USE_MP
void	MP_StartAP(int CPU);
void	MP_SendIPIVector(int CPU, Uint8 Vector);
void	MP_SendIPI(Uint8 APICID, int Vector, int DeliveryMode);
#endif
void	Proc_IdleThread(void *Ptr);
//void	Proc_Start(void);
//tThread	*Proc_GetCurThread(void);
void	Proc_ChangeStack(void);
// int	Proc_NewKThread(void (*Fcn)(void*), void *Data);
// int	Proc_Clone(Uint *Err, Uint Flags);
Uint	Proc_MakeUserStack(void);
//void	Proc_StartUser(Uint Entrypoint, Uint *Bases, int ArgC, char **ArgV, char **EnvP, int DataSize);
void	Proc_StartProcess(Uint16 SS, Uint Stack, Uint Flags, Uint16 CS, Uint IP) NORETURN;
void	Proc_CallUser(Uint32 UserIP, Uint32 UserSP, const void *StackData, size_t StackDataLen);
//void	Proc_CallFaultHandler(tThread *Thread);
//void	Proc_DumpThreadCPUState(tThread *Thread);
void	Proc_Scheduler(int CPU);

// === GLOBALS ===
// --- Multiprocessing ---
#if USE_MP
volatile int	giNumInitingCPUs = 0;
volatile Uint32	giMP_TimerCount;	// Start Count for Local APIC Timer
tAPIC	*gpMP_LocalAPIC = NULL;
Uint8	gaAPIC_to_CPU[256] = {0};
 int	giProc_BootProcessorID = 0;
tTSS	gaTSSs[MAX_CPUS];	// TSS Array
#endif
tCPU	gaCPUs[MAX_CPUS] = {
	{.Current = &gThreadZero}
	};
tTSS	*gTSSs = NULL;	// Pointer to TSS array
tTSS	gTSS0 = {0};
// --- Error Recovery ---
char	gaDoubleFaultStack[1024] __attribute__ ((section(".padata")));
tTSS	gDoubleFault_TSS = {
	.ESP0 = (Uint)&gaDoubleFaultStack[1024],
	.SS0 = 0x10,
	.CR3 = (Uint)gaInitPageDir - KERNEL_BASE,
	.EIP = (Uint)Isr8,
	.ESP = (Uint)&gaDoubleFaultStack[1024],
	.CS = 0x08,	.SS = 0x10,
	.DS = 0x10,	.ES = 0x10,
	.FS = 0x10,	.GS = 0x10,
};

// === CODE ===
/**
 * \fn void ArchThreads_Init(void)
 * \brief Starts the process scheduler
 */
void ArchThreads_Init(void)
{
	// Mark BSP as active
	gaCPUs[0].State = 2;
	
	#if USE_MP
	// -- Initialise Multiprocessing
	const void *mpfloatptr = MPTable_LocateFloatPtr();
	if( mpfloatptr )
	{
		giNumCPUs = MPTable_FillCPUs(mpfloatptr, gaCPUs, MAX_CPUS, &giProc_BootProcessorID);
		for( int i = 0; i < giNumCPUs; i ++ )
		{
			// TODO: Determine if there's an overlap
			gaAPIC_to_CPU[gaCPUs[i].APICID] = i;
		}
		gTSSs = gaTSSs;
	}
	else
	{
		Log("No MP Table was found, assuming uniprocessor");
		giNumCPUs = 1;
		gTSSs = &gTSS0;
	}
	#else
	giNumCPUs = 1;
	gTSSs = &gTSS0;
	#endif
	
	#if !DEBUG_DISABLE_DOUBLEFAULT
	// Initialise Double Fault TSS
	gGDT[5].BaseLow = (Uint)&gDoubleFault_TSS & 0xFFFF;
	gGDT[5].BaseMid = (Uint)&gDoubleFault_TSS >> 16;
	gGDT[5].BaseHi = (Uint)&gDoubleFault_TSS >> 24;
	
	// Set double fault IDT to use the new TSS
	gIDT[8].OffsetLo = 0;
	gIDT[8].CS = 5<<3;
	gIDT[8].Flags = 0x8500;
	gIDT[8].OffsetHi = 0;
	#endif
	
	// Set timer frequency
	outb(0x43, 0x34);	// Set Channel 0, Low/High, Rate Generator
	outb(0x40, TIMER_DIVISOR&0xFF);	// Low Byte of Divisor
	outb(0x40, (TIMER_DIVISOR>>8)&0xFF);	// High Byte
	
	Log_Debug("Proc", "PIT Frequency %i.%03i Hz",
		TIMER_BASE/TIMER_DIVISOR,
		((Uint64)TIMER_BASE*1000/TIMER_DIVISOR)%1000
		);
	
	#if USE_MP
	// Get the count setting for APIC timer
	Log("Determining APIC Count");
	__asm__ __volatile__ ("sti");
	while( giMP_TimerCount == 0 )	__asm__ __volatile__ ("hlt");
	__asm__ __volatile__ ("cli");
	Log("APIC Count %i", giMP_TimerCount);
	{
		Uint64	freq = giMP_TimerCount;
		freq *= TIMER_BASE;
		freq /= TIMER_DIVISOR;
		if( (freq /= 1000) < 2*1000)
			Log("Bus Frequency %i KHz", freq);
		else if( (freq /= 1000) < 2*1000)
			Log("Bus Frequency %i MHz", freq);
		else if( (freq /= 1000) < 2*1000)
			Log("Bus Frequency %i GHz", freq);
		else
			Log("Bus Frequency %i THz", freq);
	}
	
	// Initialise Normal TSS(s)
	for(int pos=0;pos<giNumCPUs;pos++)
	{
	#else
	const int pos = 0;
	#endif
		gTSSs[pos].SS0 = 0x10;
		gTSSs[pos].ESP0 = 0;	// Set properly by scheduler
		gGDT[6+pos].BaseLow = ((Uint)(&gTSSs[pos])) & 0xFFFF;
		gGDT[6+pos].BaseMid = ((Uint)(&gTSSs[pos]) >> 16) & 0xFFFF;
		gGDT[6+pos].BaseHi = ((Uint)(&gTSSs[pos])) >> 24;
	#if USE_MP
	}
	#endif
	
	// Load the BSP's TSS
	__asm__ __volatile__ ("ltr %%ax"::"a"(0x30));
	// Set Current Thread and CPU Number in DR0 and DR1
	__asm__ __volatile__ ("mov %0, %%db0"::"r"(&gThreadZero));
	__asm__ __volatile__ ("mov %0, %%db1"::"r"(0));
	
	gaCPUs[0].Current = &gThreadZero;
	gThreadZero.CurCPU = 0;
	
	gProcessZero.MemState.CR3 = (Uint)gaInitPageDir - KERNEL_BASE;
	
	// Create Per-Process Data Block
	if( MM_Allocate( (void*)MM_PPD_CFG ) == 0 )
	{
		Panic("OOM - No space for initial Per-Process Config");
	}

	// Initialise SSE support
	Proc_InitialiseSSE();
	
	// Change Stacks
	Proc_ChangeStack();
}

#if USE_MP
/**
 * \brief Start an AP
 */
void MP_StartAP(int CPU)
{
	Log_Log("Proc", "Starting AP %i (APIC %i)", CPU, gaCPUs[CPU].APICID);
	
	// Set location of AP startup code and mark for a warm restart
	*(Uint16*)(KERNEL_BASE|0x467) = (Uint)&APWait - (KERNEL_BASE|0xFFFF0);
	*(Uint16*)(KERNEL_BASE|0x469) = 0xFFFF;
	outb(0x70, 0x0F);	outb(0x71, 0x0A);	// Set warm reset flag
	MP_SendIPI(gaCPUs[CPU].APICID, 0, 5);	// Init IPI

	// Take a quick nap (20ms)
	Time_Delay(20);

	// TODO: Use a better address, preferably registered with the MM
	// - MM_AllocDMA mabye?
	// Create a far jump
	*(Uint8*)(KERNEL_BASE|0x11000) = 0xEA;	// Far JMP
	*(Uint16*)(KERNEL_BASE|0x11001) = (Uint16)(tVAddr)&APStartup + 0x10;	// IP
	*(Uint16*)(KERNEL_BASE|0x11003) = 0xFFFF;	// CS
	
	giNumInitingCPUs ++;
	
	// Send a Startup-IPI to make the CPU execute at 0x11000 (which we
	// just filled)
	MP_SendIPI(gaCPUs[CPU].APICID, 0x11, 6);	// StartupIPI
	
	tTime	timeout = now() + 2;
	while( giNumInitingCPUs && now() > timeout )
		HALT();
	
	if( giNumInitingCPUs == 0 )
		return ;
	
	// First S-IPI failed, send again
	MP_SendIPI(gaCPUs[CPU].APICID, 0x11, 6);
	timeout = now() + 2;
	while( giNumInitingCPUs && now() > timeout )
		HALT();
	if( giNumInitingCPUs == 0 )
		return ;

	Log_Notice("Proc", "CPU %i (APIC %x) didn't come up", CPU, gaCPUs[CPU].APICID);	

	// Oh dammit.
	giNumInitingCPUs = 0;
}

void MP_SendIPIVector(int CPU, Uint8 Vector)
{
	MP_SendIPI(gaCPUs[CPU].APICID, Vector, 0);
}

/**
 * \brief Send an Inter-Processor Interrupt
 * \param APICID	Processor's Local APIC ID
 * \param Vector	Argument of some kind
 * \param DeliveryMode	Type of signal
 * \note 3A 10.5 "APIC/Handling Local Interrupts"
 */
void MP_SendIPI(Uint8 APICID, int Vector, int DeliveryMode)
{
	Uint32	val;
	
	// Hi
	val = (Uint)APICID << 24;
	gpMP_LocalAPIC->ICR[1].Val = val;
	// Low (and send)
	val = ((DeliveryMode & 7) << 8) | (Vector & 0xFF);
	gpMP_LocalAPIC->ICR[0].Val = val;
}
#endif

void Proc_IdleThread(void *Ptr)
{
	tCPU	*cpu = &gaCPUs[GetCPUNum()];
	cpu->Current->ThreadName = strdup("Idle Thread");
	Threads_SetPriority( cpu->Current, -1 );	// Never called randomly
	cpu->Current->Quantum = 1;	// 1 slice quantum
	for(;;) {
		__asm__ __volatile__ ("sti");	// Make sure interrupts are enabled
		__asm__ __volatile__ ("hlt");
		Proc_Reschedule();
	}
}

/**
 * \fn void Proc_Start(void)
 * \brief Start process scheduler
 */
void Proc_Start(void)
{
	#if USE_MP
	// BSP still should run the current task
	gaCPUs[giProc_BootProcessorID].Current = &gThreadZero;
	
	__asm__ __volatile__ ("sti");
	
	// Start APs
	for( int i = 0; i < giNumCPUs; i ++ )
	{
		if(i != giProc_BootProcessorID)
			gaCPUs[i].Current = NULL;

		// Create Idle Task
		Proc_NewKThread(Proc_IdleThread, &gaCPUs[i]);
		
		// Start the AP
		if( i != giProc_BootProcessorID ) {
			MP_StartAP( i );
		}
	}
	#else
	// Create Idle Task
	Proc_NewKThread(Proc_IdleThread, &gaCPUs[0]);
	
	// Set current task
	gaCPUs[0].Current = &gThreadZero;

	// Start Interrupts (and hence scheduler)
	__asm__ __volatile__("sti");
	#endif
	MM_FinishVirtualInit();
}

/**
 * \fn tThread *Proc_GetCurThread(void)
 * \brief Gets the current thread
 */
tThread *Proc_GetCurThread(void)
{
	#if USE_MP
	return gaCPUs[ GetCPUNum() ].Current;
	#else
	return gaCPUs[ 0 ].Current;
	#endif
}

/**
 * \fn void Proc_ChangeStack(void)
 * \brief Swaps the current stack for a new one (in the proper stack reigon)
 */
void Proc_ChangeStack(void)
{
	Uint	esp, ebp;
	Uint	tmpEbp, oldEsp;
	Uint	curBase, newBase;

	__asm__ __volatile__ ("mov %%esp, %0":"=r"(esp));
	__asm__ __volatile__ ("mov %%ebp, %0":"=r"(ebp));

	oldEsp = esp;

	// Create new KStack
	newBase = MM_NewKStack();
	// Check for errors
	if(newBase == 0) {
		Panic("What the?? Unable to allocate space for initial kernel stack");
		return;
	}

	curBase = (Uint)&Kernel_Stack_Top;
	
	LOG("curBase = 0x%x, newBase = 0x%x", curBase, newBase);

	// Get ESP as a used size
	esp = curBase - esp;
	LOG("memcpy( %p, %p, 0x%x )", (void*)(newBase - esp), (void*)(curBase - esp), esp );
	// Copy used stack
	memcpy( (void*)(newBase - esp), (void*)(curBase - esp), esp );
	// Get ESP as an offset in the new stack
	esp = newBase - esp;
	// Adjust EBP
	ebp = newBase - (curBase - ebp);

	// Repair EBPs & Stack Addresses
	// Catches arguments also, but may trash stack-address-like values
	for(tmpEbp = esp; tmpEbp < newBase; tmpEbp += 4)
	{
		if(oldEsp < *(Uint*)tmpEbp && *(Uint*)tmpEbp < curBase)
			*(Uint*)tmpEbp += newBase - curBase;
	}
	
	Proc_GetCurThread()->KernelStack = newBase;
	
	__asm__ __volatile__ ("mov %0, %%esp"::"r"(esp));
	__asm__ __volatile__ ("mov %0, %%ebp"::"r"(ebp));
}

void Proc_ClearProcess(tProcess *Process)
{
	MM_ClearSpace(Process->MemState.CR3);
}

void Proc_ClearThread(tThread *Thread)
{
	if(Thread->SavedState.SSE) {
		free(Thread->SavedState.SSE);
		Thread->SavedState.SSE = NULL;
	}
}

tTID Proc_NewKThread(void (*Fcn)(void*), void *Data)
{
	Uint	esp;
	tThread	*newThread;
	
	newThread = Threads_CloneTCB(0);
	if(!newThread)	return -1;
	
	// Create new KStack
	newThread->KernelStack = MM_NewKStack();
	// Check for errors
	if(newThread->KernelStack == 0) {
		free(newThread);
		return -1;
	}

	esp = newThread->KernelStack;
	*(Uint*)(esp-=4) = (Uint)Data;	// Data (shadowed)
	*(Uint*)(esp-=4) = 1;	// Number of params
	*(Uint*)(esp-=4) = (Uint)Fcn;	// Function to call
	*(Uint*)(esp-=4) = (Uint)newThread;	// Thread ID
	
	newThread->SavedState.ESP = esp;
	newThread->SavedState.EIP = (Uint)&NewTaskHeader;
	newThread->SavedState.SSE = NULL;
//	Log("New (KThread) %p, esp = %p", newThread->SavedState.EIP, newThread->SavedState.ESP);
	
//	MAGIC_BREAK();	
	Threads_AddActive(newThread);

	return newThread->TID;
}

#if 0
tPID Proc_NewProcess(Uint Flags, void (*Fcn)(void*), size_t SaveSize, const void *Data)
{
	tThread	*newThread = Threads_CloneTCB(CLONE_VM);
	return 0;
}
#endif

/**
 * \fn int Proc_Clone(Uint *Err, Uint Flags)
 * \brief Clone the current process
 */
tPID Proc_Clone(Uint Flags)
{
	tThread	*newThread;
	tThread	*cur = Proc_GetCurThread();
	Uint	eip;

	Log_Warning("Proc", "Proc_Clone is deprecated");
	// Sanity, please
	if( !(Flags & CLONE_VM) ) {
		Log_Error("Proc", "Proc_Clone: Don't leave CLONE_VM unset, use Proc_NewKThread instead");
		return -1;
	}
	
	// New thread
	newThread = Threads_CloneTCB(Flags);
	if(!newThread)	return -1;
	ASSERT(newThread->Process);
	//ASSERT(CheckMem(newThread->Process, sizeof(tProcess)));
	//LOG("newThread->Process = %p", newThread->Process);

	newThread->KernelStack = cur->KernelStack;

	// Clone state
	eip = Proc_CloneInt(&newThread->SavedState.ESP, &newThread->Process->MemState.CR3, Flags & CLONE_NOUSER);
	if( eip == 0 ) {
		return 0;
	}
	//ASSERT(newThread->Process);
	//ASSERT(CheckMem(newThread->Process, sizeof(tProcess)));
	//LOG("newThread->Process = %p", newThread->Process);
	newThread->SavedState.EIP = eip;
	newThread->SavedState.SSE = NULL;
	newThread->SavedState.bSSEModified = 0;
	
	// Check for errors
	if( newThread->Process->MemState.CR3 == 0 ) {
		Log_Error("Proc", "Proc_Clone: MM_Clone failed");
		Threads_Delete(newThread);
		return -1;
	}

	// Add the new thread to the run queue
	Threads_AddActive(newThread);
	return newThread->TID;
}

/**
 * \fn int Proc_SpawnWorker(void)
 * \brief Spawns a new worker thread
 */
tThread *Proc_SpawnWorker(void (*Fcn)(void*), void *Data)
{
	Uint	stack_contents[4];
	LOG("(Fcn=%p,Data=%p)", Fcn, Data);
	
	// Create new thread
	tThread	*new = Threads_CloneThreadZero();
	LOG("new=%p", new);
	if(!new) {
		Warning("Proc_SpawnWorker - Out of heap space!\n");
		return NULL;
	}

	// Create the stack contents
	stack_contents[3] = (Uint)Data;
	stack_contents[2] = 1;
	stack_contents[1] = (Uint)Fcn;
	stack_contents[0] = (Uint)new;
	
	// Create a new worker stack (in PID0's address space)
	new->KernelStack = MM_NewWorkerStack(stack_contents, sizeof(stack_contents));
	LOG("new->KernelStack = %p", new->KernelStack);

	// Save core machine state
	new->SavedState.ESP = new->KernelStack - sizeof(stack_contents);
	new->SavedState.EIP = (Uint)NewTaskHeader;
	new->SavedState.SSE = NULL;
	new->SavedState.bSSEModified = 0;
	
	// Mark as active
	new->Status = THREAD_STAT_PREINIT;
	Threads_AddActive( new );
	LOG("Added to active");
	
	return new;
}

/**
 * \fn Uint Proc_MakeUserStack(void)
 * \brief Creates a new user stack
 */
Uint Proc_MakeUserStack(void)
{
	tPage	*base = (void*)(USER_STACK_TOP - USER_STACK_SZ);
	
	// Check Prospective Space
	for( Uint i = USER_STACK_SZ/PAGE_SIZE; i--; )
	{
		if( MM_GetPhysAddr( base + i ) != 0 )
		{
			Warning("Proc_MakeUserStack: Address %p in use", base + i);
			return 0;
		}
	}
	// Allocate Stack - Allocate incrementally to clean up MM_Dump output
	for( Uint i = 0; i < USER_STACK_SZ/PAGE_SIZE; i++ )
	{
		if( MM_Allocate( base + i ) == 0 )
		{
			Warning("OOM: Proc_MakeUserStack");
			return 0;
		}
	}
	
	return (tVAddr)( base + USER_STACK_SZ/PAGE_SIZE );
}

void Proc_StartUser(Uint Entrypoint, Uint Base, int ArgC, const char **ArgV, int DataSize)
{
	Uint	*stack;
	 int	i;
	const char	**envp = NULL;
	Uint16	ss, cs;
	
	// Copy data to the user stack and free original buffer
	stack = (void*)Proc_MakeUserStack();
	stack -= (DataSize+sizeof(*stack)-1)/sizeof(*stack);
	memcpy( stack, ArgV, DataSize );
	free(ArgV);
	
	// Adjust Arguments and environment
	if( DataSize )
	{
		Uint delta = (Uint)stack - (Uint)ArgV;
		ArgV = (const char**)stack;
		for( i = 0; ArgV[i]; i++ )	ArgV[i] += delta;
		envp = &ArgV[i+1];
		for( i = 0; envp[i]; i++ )	envp[i] += delta;
	}
	
	// User Mode Segments
	ss = 0x23;	cs = 0x1B;
	
	// Arguments
	*--stack = (Uint)envp;
	*--stack = (Uint)ArgV;
	*--stack = (Uint)ArgC;
	*--stack = Base;
	
	Proc_StartProcess(ss, (Uint)stack, 0x202, cs, Entrypoint);
}

void Proc_StartProcess(Uint16 SS, Uint Stack, Uint Flags, Uint16 CS, Uint IP)
{
	Uint	*stack = (void*)Stack;
	*--stack = SS;		//Stack Segment
	*--stack = Stack;	//Stack Pointer
	*--stack = Flags;	//EFLAGS (Resvd (0x2) and IF (0x20))
	*--stack = CS;		//Code Segment
	*--stack = IP;	//EIP
	//PUSHAD
	*--stack = 0xAAAAAAAA;	// eax
	*--stack = 0xCCCCCCCC;	// ecx
	*--stack = 0xDDDDDDDD;	// edx
	*--stack = 0xBBBBBBBB;	// ebx
	*--stack = 0xD1D1D1D1;	// edi
	*--stack = 0x54545454;	// esp - NOT POPED
	*--stack = 0x51515151;	// esi
	*--stack = 0xB4B4B4B4;	// ebp
	//Individual PUSHs
	*--stack = SS;	// ds
	*--stack = SS;	// es
	*--stack = SS;	// fs
	*--stack = SS;	// gs
	
	__asm__ __volatile__ (
	"mov %%eax,%%esp;\n\t"	// Set stack pointer
	"pop %%gs;\n\t"
	"pop %%fs;\n\t"
	"pop %%es;\n\t"
	"pop %%ds;\n\t"
	"popa;\n\t"
	"iret;\n\t" : : "a" (stack));
	for(;;);
}

void Proc_CallUser(Uint32 UserIP, Uint32 UserSP, const void *StackData, size_t StackDataLen)
{
	if( UserSP < StackDataLen )
		return ;
	if( !CheckMem( (void*)(UserSP - StackDataLen), StackDataLen ) )
		return ;
	memcpy( (void*)(UserSP - StackDataLen), StackData, StackDataLen );
	
	__asm__ __volatile__ (
		"mov $0x23,%%ax;\n\t"
		"mov %%ax, %%ds;\n\t"
		"mov %%ax, %%es;\n\t"
		"mov %%ax, %%fs;\n\t"
		"mov %%ax, %%gs;\n\t"
		"push $0x23;\n\t"
		"push %1;\n\t"
		"push $0x202;\n\t"
		"push $0x1B;\n\t"
		"push %0;\n\t"
		"iret;\n\t"
		:
		: "r" (UserIP), "r" (UserSP - StackDataLen)
		: "eax"
		);
	for(;;)
		;
}

/**
 * \brief Calls a signal handler in user mode
 * \note Used for signals
 */
void Proc_CallFaultHandler(tThread *Thread)
{
	// Rewinds the stack and calls the user function
	// Never returns
	Proc_ReturnToUser( Thread->FaultHandler, Thread->CurFaultNum, Thread->KernelStack );
	for(;;);
}

void Proc_DumpThreadCPUState(tThread *Thread)
{
	if( Thread->CurCPU > -1 )
	{
		 int	maxBacktraceDistance = 6;
		tRegs	*regs = NULL;
		Uint32	*stack;
		
		if( Thread->CurCPU != GetCPUNum() ) {
			Log("  Currently running");
			return ;
		}
		
		// Backtrace to find the IRQ entrypoint
		// - This will usually only be called by an IRQ, so this should
		//   work
		__asm__ __volatile__ ("mov %%ebp, %0" : "=r" (stack));
		while( maxBacktraceDistance -- )
		{
			if( !CheckMem(stack, 8) ) {
				regs = NULL;
				break;
			}
			// [ebp] = oldEbp
			// [ebp+4] = retaddr
			
			if( stack[1] == (tVAddr)&IRQCommon_handled ) {
				regs = (void*)stack[2];
				break;
			}
			
			stack = (void*)stack[0];
		}
		
		if( !regs ) {
			Log("  Unable to find IRQ Entry");
			return ;
		}
		
		Log("  at %04x:%08x [EAX:%x]", regs->cs, regs->eip, regs->eax);
		Error_Backtrace(regs->eip, regs->ebp);
		return ;
	}

	Log(" Saved = %p (SP=%p)", Thread->SavedState.EIP, Thread->SavedState.ESP);	

	tVAddr	diffFromScheduler = Thread->SavedState.EIP - (tVAddr)SwitchTasks;
	tVAddr	diffFromClone = Thread->SavedState.EIP - (tVAddr)Proc_CloneInt;
	tVAddr	diffFromSpawn = Thread->SavedState.EIP - (tVAddr)NewTaskHeader;
	
	if( diffFromClone > 0 && diffFromClone < 40 )	// When I last checked, .newTask was at .+27
	{
		Log("  Creating process");
		return ;
	}
	
	if( diffFromSpawn == 0 )
	{
		Log("  Creating thread");
		return ;
	}
	
	if( diffFromScheduler > 0 && diffFromScheduler < 128 )	// When I last checked, GetEIP was at .+0x30
	{
		// Scheduled out
		Log("  At %04x:%08x", Thread->SavedState.UserCS, Thread->SavedState.UserEIP);
		return ;
	}
	
	Log("  Just created (unknown %p)", Thread->SavedState.EIP);
}

void Proc_Reschedule(void)
{
	tThread	*nextthread, *curthread;
	 int	cpu = GetCPUNum();

	// TODO: Wait for the lock?
	if(IS_LOCKED(&glThreadListLock))        return;
	
	curthread = Proc_GetCurThread();

	nextthread = Threads_GetNextToRun(cpu, curthread);

	if(!nextthread || nextthread == curthread)
		return ;

	#if DEBUG_TRACE_SWITCH
	// HACK: Ignores switches to the idle threads
	if( nextthread->TID == 0 || nextthread->TID > giNumCPUs )
	{
		LogF("\nSwitching CPU %i to %p (%i %s) - CR3 = 0x%x, EIP = %p, ESP = %p\n",
			GetCPUNum(),
			nextthread, nextthread->TID, nextthread->ThreadName,
			nextthread->Process->MemState.CR3,
			nextthread->SavedState.EIP,
			nextthread->SavedState.ESP
			);
		LogF("OldCR3 = %P\n", curthread->Process->MemState.CR3);
	}
	#endif

	// Update CPU state
	gaCPUs[cpu].Current = nextthread;
	gaCPUs[cpu].LastTimerThread = NULL;
	gTSSs[cpu].ESP0 = nextthread->KernelStack-4;
	__asm__ __volatile__("mov %0, %%db0\n\t" : : "r"(nextthread) );

	// Save FPU/MMX/XMM/SSE state
	if( curthread && curthread->SavedState.SSE )
	{
		Proc_SaveSSE( ((Uint)curthread->SavedState.SSE + 0xF) & ~0xF );
		curthread->SavedState.bSSEModified = 0;
		Proc_DisableSSE();
	}

	if( curthread )
	{
		SwitchTasks(
			nextthread->SavedState.ESP, &curthread->SavedState.ESP,
			nextthread->SavedState.EIP, &curthread->SavedState.EIP,
			nextthread->Process->MemState.CR3
			);
	}
	else
	{
		SwitchTasks(
			nextthread->SavedState.ESP, 0,
			nextthread->SavedState.EIP, 0,
			nextthread->Process->MemState.CR3
			);
	}

	return ;
}

/**
 * \fn void Proc_Scheduler(int CPU)
 * \brief Swap current thread and clears dead threads
 */
void Proc_Scheduler(int CPU)
{
	#if USE_MP
	if( GetCPUNum() )
		gpMP_LocalAPIC->EOI.Val = 0;
	else
	#endif
		outb(0x20, 0x20);
	__asm__ __volatile__ ("sti");	

	// Call the timer update code
	Timer_CallTimers();

	#if !DEBUG_NOPREEMPT
	// If two ticks happen within the same task, and it's not an idle task, swap
	if( gaCPUs[CPU].Current->TID > giNumCPUs && gaCPUs[CPU].Current == gaCPUs[CPU].LastTimerThread )
	{
		Proc_Reschedule();
	}
	
	gaCPUs[CPU].LastTimerThread = gaCPUs[CPU].Current;
	#endif
}

// === EXPORTS ===
EXPORT(Proc_SpawnWorker);
