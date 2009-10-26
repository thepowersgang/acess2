/*
 * AcessOS Microkernel Version
 * proc.c
 */
#include <common.h>
#include <proc.h>
#include <mm_virt.h>
#include <errno.h>
#if USE_MP
# include <mp.h>
#endif

// === FLAGS ===
#define DEBUG_TRACE_SWITCH	0

// === CONSTANTS ===
#define	SWITCH_MAGIC	0xFFFACE55	// There is no code in this area
#define TIMER_DIVISOR	11931	//~100Hz

// === IMPORTS ===
extern tGDT	gGDT[];
extern Uint	GetEIP();	// start.asm
extern Uint32	gaInitPageDir[1024];	// start.asm
extern void	Kernel_Stack_Top;
extern volatile int	giThreadListLock;
extern int	giNumCPUs;
extern int	giNextTID;
extern int	giTotalTickets;
extern int	giNumActiveThreads;
extern tThread	gThreadZero;
extern tThread	*gActiveThreads;
extern tThread	*gSleepingThreads;
extern tThread	*gDeleteThreads;
extern tThread	*Threads_GetNextToRun(int CPU);
extern void	Threads_Dump();
extern tThread	*Threads_CloneTCB(Uint *Err, Uint Flags);
extern void	Isr7();

// === PROTOTYPES ===
void	ArchThreads_Init();
tThread	*Proc_GetCurThread();
void	Proc_ChangeStack();
 int	Proc_Clone(Uint *Err, Uint Flags);
void	Proc_Scheduler();

// === GLOBALS ===
// --- Current State ---
#if USE_MP
tThread	*gCurrentThread[MAX_CPUS] = {NULL};
#else
tThread	*gCurrentThread = NULL;
#endif
// --- Multiprocessing ---
#if USE_MP
tMPInfo	*gMPTable = NULL;
#endif
#if USE_PAE
Uint32	*gPML4s[4] = NULL;
#endif
tTSS	*gTSSs = NULL;
#if !USE_MP
tTSS	gTSS0 = {0};
#endif
// --- Error Recovery ---
char	gaDoubleFaultStack[1024];
tTSS	gDoubleFault_TSS = {
	.ESP0 = (Uint)&gaDoubleFaultStack[1023],
	.SS0 = 0x10,
	.EIP = (Uint)Isr7
};

// === CODE ===
/**
 * \fn void ArchThreads_Init()
 * \brief Starts the process scheduler
 */
void ArchThreads_Init()
{
	Uint	pos = 0;
	#if USE_MP
	// -- Initialise Multiprocessing
	// Find MP Floating Table
	// - EBDA
	for(pos = KERNEL_BASE|0x9FC00; pos < (KERNEL_BASE|0xA0000); pos += 16) {
		if( *(Uint*)(pos) == MPTABLE_IDENT ) {
			if(ByteSum( (void*)pos, sizeof(tMPInfo) ) != 0)	continue;
			gMPTable = (void*)pos;
			break;
		}
	}
	// - Last KiB
	if(!gMPTable) {
		
	}
	// - BIOS ROM
	if(!gMPTable) {
		for(pos = KERNEL_BASE|0xF0000; pos < (KERNEL_BASE|0x100000); pos += 16) {
			if( *(Uint*)(pos) == MPTABLE_IDENT ) {
				if(ByteSum( (void*)pos, sizeof(tMPInfo) ) != 0)	continue;
				gMPTable = (void*)pos;
				break;
			}
		}
	}
	
	// If the MP Table Exists, parse it
	if(gMPTable)
	{
		Panic("Uh oh... MP Table Parsing is unimplemented\n");
	} else {
	#endif
		giNumCPUs = 1;
		gTSSs = &gTSS0;
	#if USE_MP
	}
	
	// Initialise Double Fault TSS
	gGDT[5].LimitLow = sizeof(tTSS);
	gGDT[5].LimitHi = 0;
	gGDT[5].Access = 0x89;	// Type
	gGDT[5].Flags = 0x4;
	gGDT[5].BaseLow = (Uint)&gDoubleFault_TSS & 0xFFFF;
	gGDT[5].BaseMid = (Uint)&gDoubleFault_TSS >> 16;
	gGDT[5].BaseHi = (Uint)&gDoubleFault_TSS >> 24;
	
	// Initialise TSS
	for(pos=0;pos<giNumCPUs;pos++)
	{
	#else
	pos = 0;
	#endif
		gTSSs[pos].SS0 = 0x10;
		gTSSs[pos].ESP0 = 0;	// Set properly by scheduler
		gGDT[6+pos].LimitLow = sizeof(tTSS);
		gGDT[6+pos].LimitHi = 0;
		gGDT[6+pos].Access = 0x89;	// Type
		gGDT[6+pos].Flags = 0x4;
		gGDT[6+pos].BaseLow = (Uint)&gTSSs[pos] & 0xFFFF;
		gGDT[6+pos].BaseMid = (Uint)&gTSSs[pos] >> 16;
		gGDT[6+pos].BaseHi = (Uint)&gTSSs[pos] >> 24;
	#if USE_MP
	}
	for(pos=0;pos<giNumCPUs;pos++) {
	#endif
		__asm__ __volatile__ ("ltr %%ax"::"a"(0x30+pos*8));
	#if USE_MP
	}
	#endif
	
	#if USE_MP
	gCurrentThread[0] = &gThreadZero;
	#else
	gCurrentThread = &gThreadZero;
	#endif
	
	#if USE_PAE
	gThreadZero.MemState.PDP[0] = 0;
	gThreadZero.MemState.PDP[1] = 0;
	gThreadZero.MemState.PDP[2] = 0;
	#else
	gThreadZero.MemState.CR3 = (Uint)gaInitPageDir - KERNEL_BASE;
	#endif
	
	// Set timer frequency
	outb(0x43, 0x34);	// Set Channel 0, Low/High, Rate Generator
	outb(0x40, TIMER_DIVISOR&0xFF);	// Low Byte of Divisor
	outb(0x40, (TIMER_DIVISOR>>8)&0xFF);	// High Byte
	
	// Create Per-Process Data Block
	MM_Allocate(MM_PPD_CFG);
	
	// Change Stacks
	Proc_ChangeStack();
}

/**
 * \fn void Proc_Start()
 * \brief Start process scheduler
 */
void Proc_Start()
{
	// Start Interrupts (and hence scheduler)
	__asm__ __volatile__("sti");
}

/**
 * \fn tThread *Proc_GetCurThread()
 * \brief Gets the current thread
 */
tThread *Proc_GetCurThread()
{
	#if USE_MP
	return NULL;
	#else
	return gCurrentThread;
	#endif
}

/**
 * \fn void Proc_ChangeStack()
 * \brief Swaps the current stack for a new one (in the proper stack reigon)
 */
void Proc_ChangeStack()
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
	
	gCurrentThread->KernelStack = newBase;
	
	__asm__ __volatile__ ("mov %0, %%esp"::"r"(esp));
	__asm__ __volatile__ ("mov %0, %%ebp"::"r"(ebp));
}

/**
 * \fn int Proc_Clone(Uint *Err, Uint Flags)
 * \brief Clone the current process
 */
int Proc_Clone(Uint *Err, Uint Flags)
{
	tThread	*newThread;
	tThread	*cur = Proc_GetCurThread();
	Uint	eip, esp, ebp;
	
	__asm__ __volatile__ ("mov %%esp, %0": "=r"(esp));
	__asm__ __volatile__ ("mov %%ebp, %0": "=r"(ebp));
	
	newThread = Threads_CloneTCB(Err, Flags);
	if(!newThread)	return -1;
	
	// Initialise Memory Space (New Addr space or kernel stack)
	if(Flags & CLONE_VM) {
		newThread->MemState.CR3 = MM_Clone();
		newThread->KernelStack = cur->KernelStack;
	} else {
		Uint	tmpEbp, oldEsp = esp;

		// Set CR3
		newThread->MemState.CR3 = cur->MemState.CR3;

		// Create new KStack
		newThread->KernelStack = MM_NewKStack();
		// Check for errors
		if(newThread->KernelStack == 0) {
			free(newThread);
			return -1;
		}

		// Get ESP as a used size
		esp = cur->KernelStack - esp;
		// Copy used stack
		memcpy( (void*)(newThread->KernelStack - esp), (void*)(cur->KernelStack - esp), esp );
		// Get ESP as an offset in the new stack
		esp = newThread->KernelStack - esp;
		// Adjust EBP
		ebp = newThread->KernelStack - (cur->KernelStack - ebp);

		// Repair EBPs & Stack Addresses
		// Catches arguments also, but may trash stack-address-like values
		for(tmpEbp = esp; tmpEbp < newThread->KernelStack; tmpEbp += 4)
		{
			if(oldEsp < *(Uint*)tmpEbp && *(Uint*)tmpEbp < cur->KernelStack)
				*(Uint*)tmpEbp += newThread->KernelStack - cur->KernelStack;
		}
	}
	
	// Save core machine state
	newThread->SavedState.ESP = esp;
	newThread->SavedState.EBP = ebp;
	eip = GetEIP();
	if(eip == SWITCH_MAGIC) {
		outb(0x20, 0x20);	// ACK Timer and return as child
		return 0;
	}
	
	// Set EIP as parent
	newThread->SavedState.EIP = eip;
	
	// Lock list and add to active
	Threads_AddActive(newThread);
	
	return newThread->TID;
}

/**
 * \fn int Proc_SpawnWorker()
 * \brief Spawns a new worker thread
 */
int Proc_SpawnWorker()
{
	tThread	*new, *cur;
	Uint	eip, esp, ebp;
	
	cur = Proc_GetCurThread();
	
	// Create new thread
	new = malloc( sizeof(tThread) );
	if(!new) {
		Warning("Proc_SpawnWorker - Out of heap space!\n");
		return -1;
	}
	memcpy(new, &gThreadZero, sizeof(tThread));
	// Set Thread ID
	new->TID = giNextTID++;
	// Set kernel stack
	new->KernelStack = MM_NewWorkerStack();

	// Get ESP and EBP based in the new stack
	__asm__ __volatile__ ("mov %%esp, %0": "=r"(esp));
	__asm__ __volatile__ ("mov %%ebp, %0": "=r"(ebp));
	esp = cur->KernelStack - (new->KernelStack - esp);
	ebp = new->KernelStack - (cur->KernelStack - ebp);	
	
	// Save core machine state
	new->SavedState.ESP = esp;
	new->SavedState.EBP = ebp;
	eip = GetEIP();
	if(eip == SWITCH_MAGIC) {
		outb(0x20, 0x20);	// ACK Timer and return as child
		return 0;
	}
	
	// Set EIP as parent
	new->SavedState.EIP = eip;
	
	return new->TID;
}

/**
 * \fn Uint Proc_MakeUserStack()
 * \brief Creates a new user stack
 */
Uint Proc_MakeUserStack()
{
	 int	i;
	Uint	base = USER_STACK_TOP - USER_STACK_SZ;
	
	// Check Prospective Space
	for( i = USER_STACK_SZ >> 12; i--; )
		if( MM_GetPhysAddr( base + (i<<12) ) != 0 )
			break;
	
	if(i != -1)	return 0;
	
	// Allocate Stack - Allocate incrementally to clean up MM_Dump output
	for( i = 0; i < USER_STACK_SZ/4069; i++ )
		MM_Allocate( base + (i<<12) );
	
	return base + USER_STACK_SZ;
}


/**
 * \fn void Proc_StartUser(Uint Entrypoint, Uint *Bases, int ArgC, char **ArgV, char **EnvP, int DataSize)
 * \brief Starts a user task
 */
void Proc_StartUser(Uint Entrypoint, Uint *Bases, int ArgC, char **ArgV, char **EnvP, int DataSize)
{
	Uint	*stack = (void*)Proc_MakeUserStack();
	 int	i;
	Uint	delta;
	Uint16	ss, cs;
	
	LOG("stack = 0x%x", stack);
	
	// Copy Arguments
	stack = (void*)( (Uint)stack - DataSize );
	memcpy( stack, ArgV, DataSize );
	
	// Adjust Arguments and environment
	delta = (Uint)stack - (Uint)ArgV;
	ArgV = (char**)stack;
	for( i = 0; ArgV[i]; i++ )	ArgV[i] += delta;
	i ++;
	EnvP = &ArgV[i];
	for( i = 0; EnvP[i]; i++ )	EnvP[i] += delta;
	
	// User Mode Segments
	ss = 0x23;	cs = 0x1B;
	
	// Arguments
	*--stack = (Uint)EnvP;
	*--stack = (Uint)ArgV;
	*--stack = (Uint)ArgC;
	while(*Bases)
		*--stack = *Bases++;
	*--stack = 0;	// Return Address
	delta = (Uint)stack;	// Reuse delta to save SP
	
	*--stack = ss;		//Stack Segment
	*--stack = delta;	//Stack Pointer
	*--stack = 0x0202;	//EFLAGS (Resvd (0x2) and IF (0x20))
	*--stack = cs;		//Code Segment
	*--stack = Entrypoint;	//EIP
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
	*--stack = ss;	// ds
	*--stack = ss;	// es
	*--stack = ss;	// fs
	*--stack = ss;	// gs
	
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

/**
 * \fn int Proc_Demote(Uint *Err, int Dest, tRegs *Regs)
 * \brief Demotes a process to a lower permission level
 * \param Err	Pointer to user's errno
 * \param Dest	New Permission Level
 * \param Regs	Pointer to user's register structure
 */
int Proc_Demote(Uint *Err, int Dest, tRegs *Regs)
{
	 int	cpl = Regs->cs & 3;
	// Sanity Check
	if(Dest > 3 || Dest < 0) {
		*Err = -EINVAL;
		return -1;
	}
	
	// Permission Check
	if(cpl > Dest) {
		*Err = -EACCES;
		return -1;
	}
	
	// Change the Segment Registers
	Regs->cs = (((Dest+1)<<4) | Dest) - 8;
	Regs->ss = ((Dest+1)<<4) | Dest;
	// Check if the GP Segs are GDT, then change them
	if(!(Regs->ds & 4))	Regs->ds = ((Dest+1)<<4) | Dest;
	if(!(Regs->es & 4))	Regs->es = ((Dest+1)<<4) | Dest;
	if(!(Regs->fs & 4))	Regs->fs = ((Dest+1)<<4) | Dest;
	if(!(Regs->gs & 4))	Regs->gs = ((Dest+1)<<4) | Dest;
	
	return 0;
}

/**
 * \fn void Proc_Scheduler(int CPU)
 * \brief Swap current thread and clears dead threads
 */
void Proc_Scheduler(int CPU)
{
	Uint	esp, ebp, eip;
	tThread	*thread;
	
	// If the spinlock is set, let it complete
	if(giThreadListLock)	return;
	
	// Clear Delete Queue
	while(gDeleteThreads)
	{
		thread = gDeleteThreads->Next;
		if(gDeleteThreads->IsLocked) {	// Only free if structure is unused
			gDeleteThreads->Status = THREAD_STAT_NULL;
			free( gDeleteThreads );
		}
		gDeleteThreads = thread;
	}
	
	// Check if there is any tasks running
	if(giNumActiveThreads == 0) {
		Log("No Active threads, sleeping");
		__asm__ __volatile__ ("hlt");
		return;
	}
	
	// Reduce remaining quantum and continue timeslice if non-zero
	if(gCurrentThread->Remaining--)	return;
	// Reset quantum for next call
	gCurrentThread->Remaining = gCurrentThread->Quantum;
	
	// Get machine state
	__asm__ __volatile__ ("mov %%esp, %0":"=r"(esp));
	__asm__ __volatile__ ("mov %%ebp, %0":"=r"(ebp));
	eip = GetEIP();
	if(eip == SWITCH_MAGIC)	return;	// Check if a switch happened
	
	// Save machine state
	gCurrentThread->SavedState.ESP = esp;
	gCurrentThread->SavedState.EBP = ebp;
	gCurrentThread->SavedState.EIP = eip;
	
	// Get next thread
	thread = Threads_GetNextToRun(CPU);
	
	// Error Check
	if(thread == NULL) {
		Warning("Hmm... Threads_GetNextToRun returned NULL, I don't think this should happen.\n");
		return;
	}
	
	#if DEBUG_TRACE_SWITCH
	Log("Switching to task %i, CR3 = 0x%x, EIP = %p",
		thread->TID,
		thread->MemState.CR3,
		thread->SavedState.EIP
		);
	#endif
	
	// Set current thread
	gCurrentThread = thread;
	
	// Update Kernel Stack pointer
	gTSSs[CPU].ESP0 = thread->KernelStack;
	
	// Set address space
	__asm__ __volatile__ ("mov %0, %%cr3"::"a"(gCurrentThread->MemState.CR3));
	// Switch threads
	__asm__ __volatile__ (
		"mov %1, %%esp\n\t"
		"mov %2, %%ebp\n\t"
		"jmp *%3" : :
		"a"(SWITCH_MAGIC), "b"(gCurrentThread->SavedState.ESP),
		"d"(gCurrentThread->SavedState.EBP), "c"(gCurrentThread->SavedState.EIP));
	for(;;);	// Shouldn't reach here
}
