/*
 * AcessOS Microkernel Version
 * proc.c
 */
#include <common.h>
#include <proc.h>
#include <mm_virt.h>
#include <errno.h>
#include <mp.h>

// === CONSTANTS ===
#define	RANDOM_SEED	0xACE55051
#define	SWITCH_MAGIC	0xFFFACE55	// There is no code in this area
#define	DEFAULT_QUANTUM	10
#define	DEFAULT_TICKETS	5
#define MAX_TICKETS		10
#define TIMER_DIVISOR	11931	//~100Hz

// === MACROS ===
#define TIMER_BASE	1193182 //Hz
#define MS_PER_TICK_WHOLE	(1000*(TIMER_DIVISOR)/(TIMER_BASE))
#define MS_PER_TICK_FRACT	((Uint64)(1000*TIMER_DIVISOR-((Uint64)MS_PER_TICK_WHOLE)*TIMER_BASE)*(0x80000000/TIMER_BASE))

// === IMPORTS ===
extern Uint	GetEIP();	// start.asm
extern Uint32	gaInitPageDir[1024];	// start.asm
extern void	Kernel_Stack_Top;

// === PROTOTYPES ===
void	Proc_Start();
static tThread	*Proc_int_GetPrevThread(tThread **List, tThread *Thread);
void	Proc_Scheduler();
Uint	rand();

// === GLOBALS ===
// -- Core Thread --
tThread	gThreadZero = {
	NULL, 0,	// Next, Lock
	THREAD_STAT_ACTIVE,	// Status
	0, 0,	// TID, TGID
	0, 0,	// UID, GID
	"ThreadZero",	// Name
	0, 0, 0,	// ESP, EBP, EIP (Set on switch)
	(Uint)&gaInitPageDir-KERNEL_BASE,	// CR3
	(Uint)&Kernel_Stack_Top,	// Kernel Stack (Unused as it it PL0)
	NULL, NULL,	// Messages, Last Message
	DEFAULT_QUANTUM, DEFAULT_QUANTUM,	// Quantum, Remaining
	DEFAULT_TICKETS
	};
// -- Processes --
// --- Locks ---
 int	giThreadListLock = 0;	///\note NEVER use a heap function while locked
// --- Current State ---
tThread	*gCurrentThread = NULL;
 int	giNumActiveThreads = 0;
 int	giTotalTickets = 0;
Uint	giNextTID = 1;
// --- Thread Lists ---
tThread	*gActiveThreads = NULL;		// Currently Running Threads
tThread	*gSleepingThreads = NULL;	// Sleeping Threads
tThread	*gDeleteThreads = NULL;		// Threads to delete
// --- Timekeeping ---
Uint64	giTicks = 0;
Uint64	giTimestamp = 0;
Uint64	giPartMiliseconds = 0;
// --- Multiprocessing ---
 int	giNumCPUs = 1;
tMPInfo	*gMPTable = NULL;
tTSS	*gTSSs = NULL;
tTSS	gTSS0 = {0};

// === CODE ===
/**
 * \fn void Proc_Start()
 * \brief Starts the process scheduler
 */
void Proc_Start()
{
	Uint	pos;
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
		giNumCPUs = 1;
		gTSSs = &gTSS0;
	}
	
	// Initialise TSS
	for(pos=0;pos<giNumCPUs;pos++)
	{
		gTSSs[pos].SS0 = 0x10;
		gTSSs[pos].ESP0 = 0;	// Set properly by scheduler
		gGDT[9+pos].LimitLow = sizeof(tTSS);
		gGDT[9+pos].LimitHi = 0;
		gGDT[9+pos].Access = 0x89;	// Type
		gGDT[9+pos].Flags = 0x4;
		gGDT[9+pos].BaseLow = (Uint)&gTSSs[pos] & 0xFFFF;
		gGDT[9+pos].BaseMid = (Uint)&gTSSs[pos] >> 16;
		gGDT[9+pos].BaseHi = (Uint)&gTSSs[pos] >> 24;
	}
	for(pos=0;pos<giNumCPUs;pos++) {
		__asm__ __volatile__ ("ltr %%ax"::"a"(0x48+pos*8));
	}
	
	// Set timer frequency
	outb(0x43, 0x34);	// Set Channel 0, Low/High, Rate Generator
	outb(0x40, TIMER_DIVISOR&0xFF);	// Low Byte of Divisor
	outb(0x40, (TIMER_DIVISOR>>8)&0xFF);	// High Byte
	
	// Clear timestamp
	giTimestamp = 0;
	giPartMiliseconds = 0;
	giTicks = 0;
	
	// Create Initial Task
	gActiveThreads = &gThreadZero;
	gCurrentThread = &gThreadZero;
	giTotalTickets = gThreadZero.NumTickets;
	giNumActiveThreads = 1;
	
	// Create Idle Task
	if(Proc_Clone(0, 0) == 0)
	{
		gCurrentThread->ThreadName = "Idle Thread";
		Proc_SetTickets(0);	// Never called randomly
		gCurrentThread->Quantum = 1;	// 1 slice quantum
		for(;;)	__asm__ __volatile__ ("hlt");	// Just yeilds
	}
	
	// Start Interrupts (and hence scheduler)
	__asm__ __volatile__("sti");
}

/**
 * \fn int Proc_Clone(Uint *Err, Uint Flags)
 * \brief Clone the current process
 */
int Proc_Clone(Uint *Err, Uint Flags)
{
	tThread	*newThread;
	Uint	eip, esp, ebp;
	
	__asm__ __volatile__ ("mov %%esp, %0": "=r"(esp));
	__asm__ __volatile__ ("mov %%ebp, %0": "=r"(ebp));
	
	// Create new thread structure
	newThread = malloc( sizeof(tThread) );
	if(!newThread) {
		Warning("Proc_Clone - Out of memory when creating thread\n");
		*Err = -ENOMEM;
		return -1;
	}
	// Base new thread on old
	memcpy(newThread, gCurrentThread, sizeof(tThread));
	// Initialise Memory Space (New Addr space or kernel stack)
	if(Flags & CLONE_VM) {
		newThread->TGID = newThread->TID;
		newThread->CR3 = MM_Clone();
	} else {
		Uint	tmpEbp, oldEsp = esp;

		// Create new KStack
		newThread->KernelStack = MM_NewKStack();
		// Check for errors
		if(newThread->KernelStack == 0) {
			free(newThread);
			return -1;
		}

		// Get ESP as a used size
		esp = gCurrentThread->KernelStack - esp;
		// Copy used stack
		memcpy( (void*)(newThread->KernelStack - esp), (void*)(gCurrentThread->KernelStack - esp), esp );
		// Get ESP as an offset in the new stack
		esp = newThread->KernelStack - esp;
		// Adjust EBP
		ebp = newThread->KernelStack - (gCurrentThread->KernelStack - ebp);

		// Repair EBPs & Stack Addresses
		#if 0
		tmpEbp = ebp;
		while(oldEsp < *(Uint*)tmpEbp && *(Uint*)tmpEbp < gCurrentThread->KernelStack)
		{
			*(Uint*)tmpEbp += newThread->KernelStack - gCurrentThread->KernelStack;
			tmpEbp = *(Uint*)tmpEbp;
		}
		#else	// Catches arguments also, but may trash stack-address-like values
		for(tmpEbp = esp; tmpEbp < newThread->KernelStack; tmpEbp += 4)
		{
			if(oldEsp < *(Uint*)tmpEbp && *(Uint*)tmpEbp < gCurrentThread->KernelStack)
				*(Uint*)tmpEbp += newThread->KernelStack - gCurrentThread->KernelStack;
		}
		#endif
	}

	// Set Pointer, Spinlock and TID
	newThread->Next = NULL;
	newThread->IsLocked = 0;
	newThread->TID = giNextTID++;

	// Clear message list (messages are not inherited)
	newThread->Messages = NULL;
	newThread->LastMessage = NULL;
	
	// Set remaining (sheduler expects remaining to be correct)
	newThread->Remaining = newThread->Quantum;
	
	// Save core machine state
	newThread->ESP = esp;
	newThread->EBP = ebp;
	eip = GetEIP();
	if(eip == SWITCH_MAGIC) {
		outb(0x20, 0x20);	// ACK Timer and return as child
		return 0;
	}
	
	// Set EIP as parent
	newThread->EIP = eip;
	
	//Log(" Proc_Clone: giTimestamp = %i.%07i", (Uint)giTimestamp, (Uint)giPartMiliseconds/214);
	
	// Lock list and add to active
	LOCK( &giThreadListLock );
	newThread->Next = gActiveThreads;
	gActiveThreads = newThread;
	giNumActiveThreads ++;
	giTotalTickets += newThread->NumTickets;
	RELEASE( &giThreadListLock );
	
	return newThread->TID;
}

/**
 * \fn void Proc_Exit()
 * \brief Kill the current process
 */
void Proc_Exit()
{
	tThread	*thread;
	tMsg	*msg;
	
	///\note Double lock is needed due to overlap of locks
	
	// Lock thread (stop us recieving messages)
	LOCK( &gCurrentThread->IsLocked );
	
	// Lock thread list
	LOCK( &giThreadListLock );
	
	// Get previous thread on list
	thread = Proc_int_GetPrevThread( &gActiveThreads, gCurrentThread );
	if(!thread) {
		Warning("Proc_Exit - Current thread is not on the active queue");
		return;
	}
	
	// Clear Message Queue
	while( gCurrentThread->Messages )
	{
		msg = gCurrentThread->Messages->Next;
		free( gCurrentThread->Messages );
		gCurrentThread->Messages = msg;
	}
	
	gCurrentThread->Remaining = 0;	// Clear Remaining Quantum
	gCurrentThread->Quantum = 0;	// Clear Quantum to indicate dead thread
	thread->Next = gCurrentThread->Next;	// Remove from active
	
	// Add to delete queue
	if(gDeleteThreads) {
		gCurrentThread->Next = gDeleteThreads;
		gDeleteThreads = gCurrentThread;
	} else {
		gCurrentThread->Next = NULL;
		gDeleteThreads = gCurrentThread;
	}
	
	giNumActiveThreads --;
	giTotalTickets -= gCurrentThread->NumTickets;
	
	// Mark thread as sleeping
	gCurrentThread->Status = THREAD_STAT_DEAD;
	
	// Release spinlocks
	RELEASE( &gCurrentThread->IsLocked );	// Released first so that it IS released
	RELEASE( &giThreadListLock );
	__asm__ __volatile__ ("hlt");
}

/**
 * \fn void Proc_Yield()
 * \brief Yield remainder of timeslice
 */
void Proc_Yield()
{
	gCurrentThread->Quantum = 0;
	__asm__ __volatile__ ("hlt");
}

/**
 * \fn void Proc_Sleep()
 * \brief Take the current process off the run queue
 */
void Proc_Sleep()
{
	tThread *thread;
	
	//Log("Proc_Sleep: %i going to sleep", gCurrentThread->TID);
	
	// Acquire Spinlock
	LOCK( &giThreadListLock );
	
	// Get thread before current thread
	thread = Proc_int_GetPrevThread( &gActiveThreads, gCurrentThread );
	if(!thread) {
		Warning("Proc_Sleep - Current thread is not on the active queue");
		return;
	}
	
	// Don't sleep if there is a message waiting
	if( gCurrentThread->Messages ) {
		RELEASE( &giThreadListLock );
		return;
	}
	
	// Unset remaining timeslices (force a task switch on timer fire)
	gCurrentThread->Remaining = 0;
	
	// Remove from active list
	thread->Next = gCurrentThread->Next;
	
	// Add to Sleeping List (at the top)
	gCurrentThread->Next = gSleepingThreads;
	gSleepingThreads = gCurrentThread;
	
	// Reduce the active count & ticket count
	giNumActiveThreads --;
	giTotalTickets -= gCurrentThread->NumTickets;
	
	// Mark thread as sleeping
	gCurrentThread->Status = THREAD_STAT_SLEEPING;
	
	// Release Spinlock
	RELEASE( &giThreadListLock );
	
	__asm__ __volatile__ ("hlt");
}

/**
 * \fn void Thread_Wake( tThread *Thread )
 * \brief Wakes a sleeping/waiting thread up
 */
void Thread_Wake(tThread *Thread)
{
	tThread	*prev;
	switch(Thread->Status)
	{
	case THREAD_STAT_ACTIVE:	break;
	case THREAD_STAT_SLEEPING:
		LOCK( &giThreadListLock );
		prev = Proc_int_GetPrevThread(&gSleepingThreads, Thread);
		prev->Next = Thread->Next;	// Remove from sleeping queue
		Thread->Next = gActiveThreads;	// Add to active queue
		gActiveThreads = Thread;
		Thread->Status = THREAD_STAT_ACTIVE;
		RELEASE( &giThreadListLock );
		break;
	case THREAD_STAT_WAITING:
		Warning("Thread_Wake - Waiting threads are not currently supported");
		break;
	case THREAD_STAT_DEAD:
		Warning("Thread_Wake - Attempt to wake dead thread (%i)", Thread->TID);
		break;
	default:
		Warning("Thread_Wake - Unknown process status (%i)\n", Thread->Status);
		break;
	}
}

/**
 * \fn int Proc_Demote(Uint *Err, int Dest, tRegs *Regs)
 * \brief Demotes a process to a lower permission level
 * \param Err	Pointer to user's errno
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
 * \fn void Proc_SetTickets(int Num)
 * \brief Sets the 'priority' of a task
 */
void Proc_SetTickets(int Num)
{
	if(Num < 0)	return;
	if(Num > MAX_TICKETS)	Num = MAX_TICKETS;
	
	Log("Proc_SetTickets: (Num=%i)", Num);
	Log(" Proc_SetTickets: giTotalTickets = %i", giTotalTickets);
	LOCK( &giThreadListLock );
	giTotalTickets -= gCurrentThread->NumTickets;
	gCurrentThread->NumTickets = Num;
	giTotalTickets += Num;
	RELEASE( &giThreadListLock );
	Log(" Proc_SetTickets: giTotalTickets = %i", giTotalTickets);
	Log("Proc_SetTickets: RETURN", giTotalTickets);
}

/**
 * \fn tThread *Proc_GetThread(Uint TID)
 * \brief Gets a thread given its TID
 */
tThread *Proc_GetThread(Uint TID)
{
	tThread *thread;
	
	// Search Active List
	for(thread = gActiveThreads;
		thread;
		thread = thread->Next)
	{
		if(thread->TID == TID)
			return thread;
	}
	
	// Search Sleeping List
	for(thread = gSleepingThreads;
		thread;
		thread = thread->Next)
	{
		if(thread->TID == TID)
			return thread;
	}
	
	return NULL;
}

/**
 * \fn static tThread *Proc_int_GetPrevThread(tThread *List, tThread *Thread)
 * \brief Gets the previous entry in a thead linked list
 */
static tThread *Proc_int_GetPrevThread(tThread **List, tThread *Thread)
{
	tThread *ret;
	// First Entry
	if(*List == Thread) {
		return (tThread*)List;
	} else {
		for(ret = *List;
			ret->Next && ret->Next != Thread;
			ret = ret->Next
			);
		// Error if the thread is not on the list
		if(!ret->Next || ret->Next != Thread) {
			return NULL;
		}
	}
	return ret;
}

/**
 * \fn void Proc_DumpThreads()
 */
void Proc_DumpThreads()
{
	tThread	*thread;
	
	Log("Active Threads:");
	for(thread=gActiveThreads;thread;thread=thread->Next)
	{
		Log(" %i (%i) - %s", thread->TID, thread->TGID, thread->ThreadName);
		Log("  %i Tickets, Quantum %i", thread->NumTickets, thread->Quantum);
		Log("  CR3 0x%x, KStack 0x%x", thread->CR3, thread->KernelStack);
	}
	Log("Sleeping Threads:");
	for(thread=gSleepingThreads;thread;thread=thread->Next)
	{
		Log(" %i (%i) - %s", thread->TID, thread->TGID, thread->ThreadName);
		Log("  %i Tickets, Quantum %i", thread->NumTickets, thread->Quantum);
		Log("  CR3 0x%x, KStack 0x%x", thread->CR3, thread->KernelStack);
	}
}

/**
 * \fn void Proc_Scheduler(int CPU)
 * \brief Swap current task
 */
void Proc_Scheduler(int CPU)
{
	Uint	esp, ebp, eip;
	Uint	number, ticket;
	tThread	*thread;
	
	// Increment Timestamps
	giTicks ++;
	giTimestamp += MS_PER_TICK_WHOLE;
	giPartMiliseconds += MS_PER_TICK_FRACT;
	if(giPartMiliseconds > 0x80000000) {
		giTimestamp ++;
		giPartMiliseconds -= 0x80000000;
	}
	
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
		Log("No Active threads, sleeping\n");
		__asm__ __volatile__ ("hlt");
		return;
	}
	
	// Reduce remaining quantum
	if(gCurrentThread->Remaining--)	return;
	// Reset quantum for next call
	gCurrentThread->Remaining = gCurrentThread->Quantum;
	
	// Get machine state
	__asm__ __volatile__ ("mov %%esp, %0":"=r"(esp));
	__asm__ __volatile__ ("mov %%ebp, %0":"=r"(ebp));
	eip = GetEIP();
	if(eip == SWITCH_MAGIC)	return;	// Check if a switch happened
	
	// Save machine state
	gCurrentThread->ESP = esp;
	gCurrentThread->EBP = ebp;
	gCurrentThread->EIP = eip;
	
	// Special case: 1 thread
	if(giNumActiveThreads == 1)
	{
		// Check if a switch is needed (NumActive can be 1 after a sleep)
		if(gActiveThreads == gCurrentThread)	return;
		// Switch processes
		gCurrentThread = gActiveThreads;
		goto performSwitch;
	}
	
	// Get the ticket number
	ticket = number = rand() % giTotalTickets;
	
	//Log(" Proc_Scheduler: number = 0x%x\n", number);
	
	// Find the next thread
	for(thread=gActiveThreads;thread;thread=thread->Next)
	{
		if(thread->NumTickets > number)	break;
		number -= thread->NumTickets;
	}
	
	// Error Check
	if(thread == NULL)
	{
		number = 0;
		for(thread=gActiveThreads;thread;thread=thread->Next)
			number += thread->NumTickets;
		Panic("Bookeeping Failed - giTotalTicketCount (%i) != true count (%i)",
			giTotalTickets, number);
	}
	
	// Set current thread
	gCurrentThread = thread;
	
	// Update Kernel Stack pointer
	gTSSs[CPU].ESP0 = thread->KernelStack;
	
performSwitch:
	// Set address space
	MM_SetCR3( gCurrentThread->CR3 );
	// Switch threads
	__asm__ __volatile__ (
		"mov %1, %%esp\n\t"
		"mov %2, %%ebp\n\t"
		"jmp *%3" : :
		"a"(SWITCH_MAGIC), "b"(gCurrentThread->ESP),
		"d"(gCurrentThread->EBP), "c"(gCurrentThread->EIP));
	for(;;);	// Shouldn't reach here
}

/**
 * \fn Uint rand()
 * \brief Pseudo random number generator
 * \note Unknown effectiveness (made up on the spot)
 */
Uint rand()
{
	static Uint	randomState = RANDOM_SEED;
	Uint	ret = randomState;
	 int	roll = randomState & 31;
	randomState = (randomState << roll) | (randomState >> (32-roll));
	randomState ^= 0x9A3C5E78;
	return ret;
}
