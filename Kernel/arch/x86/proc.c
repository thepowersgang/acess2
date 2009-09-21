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

// === CONSTANTS ===
#define	RANDOM_SEED	0xACE55052
#define	SWITCH_MAGIC	0xFFFACE55	// There is no code in this area
#define	DEFAULT_QUANTUM	10
#define	DEFAULT_TICKETS	5
#define MAX_TICKETS		10
#define TIMER_DIVISOR	11931	//~100Hz

// === IMPORTS ===
extern tGDT	gGDT[];
extern Uint	GetEIP();	// start.asm
extern Uint32	gaInitPageDir[1024];	// start.asm
extern void	Kernel_Stack_Top;

// === PROTOTYPES ===
void	Proc_Start();
void	Proc_ChangeStack();
 int	Proc_Clone(Uint *Err, Uint Flags);
void	Proc_Exit();
void	Proc_Yield();
void	Proc_Sleep();
static tThread	*Proc_int_GetPrevThread(tThread **List, tThread *Thread);
void	Proc_Scheduler();
Sint64	now();
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
	#if USE_PAE
	{0,0,0},	// PML4 Entries
	#else
	(Uint)&gaInitPageDir-KERNEL_BASE,	// CR3
	#endif
	(Uint)&Kernel_Stack_Top,	// Kernel Stack (Unused as it is PL0)
	NULL, NULL,	// Messages, Last Message
	DEFAULT_QUANTUM, DEFAULT_QUANTUM,	// Quantum, Remaining
	DEFAULT_TICKETS,
	{0}	// Default config to zero
	};
// -- Processes --
// --- Locks ---
volatile int	giThreadListLock = 0;	///\note NEVER use a heap function while locked
// --- Current State ---
#if USE_MP
tThread	**gCurrentThread = NULL;
# define CUR_THREAD	gCurrentThread[0]
#else
tThread	*gCurrentThread = NULL;
# define CUR_THREAD	gCurrentThread
#endif
volatile int	giNumActiveThreads = 0;
volatile int	giTotalTickets = 0;
volatile Uint	giNextTID = 1;
// --- Thread Lists ---
tThread	*gActiveThreads = NULL;		// Currently Running Threads
tThread	*gSleepingThreads = NULL;	// Sleeping Threads
tThread	*gDeleteThreads = NULL;		// Threads to delete
// --- Multiprocessing ---
 int	giNumCPUs = 1;
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

// === CODE ===
/**
 * \fn void Proc_Start()
 * \brief Starts the process scheduler
 */
void Proc_Start()
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
	
	// Initialise TSS
	for(pos=0;pos<giNumCPUs;pos++)
	{
	#else
	pos = 0;
	#endif
		gTSSs[pos].SS0 = 0x10;
		gTSSs[pos].ESP0 = 0;	// Set properly by scheduler
		gGDT[5+pos].LimitLow = sizeof(tTSS);
		gGDT[5+pos].LimitHi = 0;
		gGDT[5+pos].Access = 0x89;	// Type
		gGDT[5+pos].Flags = 0x4;
		gGDT[5+pos].BaseLow = (Uint)&gTSSs[pos] & 0xFFFF;
		gGDT[5+pos].BaseMid = (Uint)&gTSSs[pos] >> 16;
		gGDT[5+pos].BaseHi = (Uint)&gTSSs[pos] >> 24;
	#if USE_MP
	}
	for(pos=0;pos<giNumCPUs;pos++) {
	#endif
		__asm__ __volatile__ ("ltr %%ax"::"a"(0x28+pos*8));
	#if USE_MP
	}
	#endif
	
	// Set timer frequency
	outb(0x43, 0x34);	// Set Channel 0, Low/High, Rate Generator
	outb(0x40, TIMER_DIVISOR&0xFF);	// Low Byte of Divisor
	outb(0x40, (TIMER_DIVISOR>>8)&0xFF);	// High Byte
	
	// Create Initial Task
	gActiveThreads = &gThreadZero;
	gCurrentThread = &gThreadZero;
	giTotalTickets = gThreadZero.NumTickets;
	giNumActiveThreads = 1;
	
	// Create Per-Process Data Block
	MM_Allocate(MM_PPD_CFG);
	
	// Change Stacks
	Proc_ChangeStack();
	
	#if 1
	// Create Idle Task
	if(Proc_Clone(0, 0) == 0)
	{
		gCurrentThread->ThreadName = "Idle Thread";
		Proc_SetTickets(0);	// Never called randomly
		gCurrentThread->Quantum = 1;	// 1 slice quantum
		for(;;)	__asm__ __volatile__ ("hlt");	// Just yeilds
	}
	#endif
	
	// Start Interrupts (and hence scheduler)
	__asm__ __volatile__("sti");
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

	curBase = gCurrentThread->KernelStack;
	
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
		// Catches arguments also, but may trash stack-address-like values
		for(tmpEbp = esp; tmpEbp < newThread->KernelStack; tmpEbp += 4)
		{
			if(oldEsp < *(Uint*)tmpEbp && *(Uint*)tmpEbp < gCurrentThread->KernelStack)
				*(Uint*)tmpEbp += newThread->KernelStack - gCurrentThread->KernelStack;
		}
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
 * \fn void Proc_SetThreadName()
 * \brief Sets the thread's name
 */
void Proc_SetThreadName(char *NewName)
{
	if( (Uint)CUR_THREAD->ThreadName > 0xC0400000 )
		free( CUR_THREAD->ThreadName );
	CUR_THREAD->ThreadName = malloc(strlen(NewName)+1);
	strcpy(CUR_THREAD->ThreadName, NewName);
}

/**
 * \fn Uint Proc_MakeUserStack()
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
 * \fn void Proc_StartUser(Uint Entrypoint, Uint Base, int ArgC, char **ArgV, char **EnvP, int DataSize)
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
	
	LOCK( &giThreadListLock );
	giTotalTickets -= gCurrentThread->NumTickets;
	gCurrentThread->NumTickets = Num;
	giTotalTickets += Num;
	//LOG("giTotalTickets = %i", giTotalTickets);
	RELEASE( &giThreadListLock );
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
	//MM_SetCR3( gCurrentThread->CR3 );
	__asm__ __volatile__ ("mov %0, %%cr3"::"a"(gCurrentThread->CR3));
	// Switch threads
	__asm__ __volatile__ (
		"mov %1, %%esp\n\t"
		"mov %2, %%ebp\n\t"
		"jmp *%3" : :
		"a"(SWITCH_MAGIC), "b"(gCurrentThread->ESP),
		"d"(gCurrentThread->EBP), "c"(gCurrentThread->EIP));
	for(;;);	// Shouldn't reach here
}

// --- Process Structure Access Functions ---
int Proc_GetPID()
{
	return gCurrentThread->TGID;
}
int Proc_GetTID()
{
	return gCurrentThread->TID;
}
int Proc_GetUID()
{
	return gCurrentThread->UID;
}
int Proc_GetGID()
{
	return gCurrentThread->GID;
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
