/*
 * Acess2
 * threads.c
 * - Common Thread Control
 */
#include <common.h>
#include <threads.h>

// === CONSTANTS ===
#define	DEFAULT_QUANTUM	10
#define	DEFAULT_TICKETS	5
#define MAX_TICKETS		10

// === IMPORTS ===
extern void	ArchThreads_Init();
extern void	Proc_Start();
extern tThread	*Proc_GetCurThread();
extern int	Proc_Clone(Uint *Err, Uint Flags);

// === PROTOTYPES ===
void	Threads_Init();
void	Threads_SetName(char *NewName);
void	Threads_SetTickets(int Num);
 int	Threads_WaitTID(int TID, int *status);
tThread	*Threads_GetThread(Uint TID);
void	Threads_AddToDelete(tThread *Thread);
tThread	*Threads_int_GetPrev(tThread **List, tThread *Thread);
void	Threads_Exit(int TID, int Status);
void	Threads_Kill(tThread *Thread, int Status);
void	Threads_Yield();
void	Threads_Sleep();
void	Threads_Wake(tThread *Thread);
 int	Threads_GetPID();
 int	Threads_GetTID();
 int	Threads_GetUID();
 int	Threads_GetGID();
void	Threads_Dump();

// === GLOBALS ===
// -- Core Thread --
tThread	gThreadZero = {
	NULL, 0,	// Next, Lock
	THREAD_STAT_ACTIVE,	// Status
	0, 	// Exit Status
	0, 0,	// TID, TGID
	0, 0,	// UID, GID
	0,	// Parent Thread ID
	"ThreadZero",	// Name
	
	0,	// Kernel Stack
	{0},	// Saved State
	{0},	// VM State
	
	0, {0}, {0},	// Signal State
	
	NULL, NULL,	// Messages, Last Message
	DEFAULT_QUANTUM, DEFAULT_QUANTUM,	// Quantum, Remaining
	DEFAULT_TICKETS,
	{0}	// Default config to zero
	};
// -- Processes --
// --- Locks ---
volatile int	giThreadListLock = 0;	///\note NEVER use a heap function while locked
// --- Current State ---
volatile int	giNumActiveThreads = 0;
volatile int	giTotalTickets = 0;
volatile Uint	giNextTID = 1;
// --- Thread Lists ---
tThread	*gActiveThreads = NULL;		// Currently Running Threads
tThread	*gSleepingThreads = NULL;	// Sleeping Threads
tThread	*gDeleteThreads = NULL;		// Threads to delete
 int	giNumCPUs = 1;

// === CODE ===
/**
 * \fn void Threads_Init()
 * \brief Initialse the thread list
 */
void Threads_Init()
{
	ArchThreads_Init();
	
	// Create Initial Task
	gActiveThreads = &gThreadZero;
	giTotalTickets = gThreadZero.NumTickets;
	giNumActiveThreads = 1;
	
	#if 1
	// Create Idle Task
	if(Proc_Clone(0, 0) == 0)
	{
		tThread	*cur = Proc_GetCurThread();
		cur->ThreadName = "Idle Thread";
		Threads_SetTickets(0);	// Never called randomly
		cur->Quantum = 1;	// 1 slice quantum
		for(;;) {
			Log("Idle");
			__asm__ __volatile__ ("hlt");	// Just yeilds
		}
	}
	#endif
	
	Proc_Start();
}

/**
 * \fn void Threads_SetName(char *NewName)
 * \brief Sets the current thread's name
 */
void Threads_SetName(char *NewName)
{
	tThread	*cur = Proc_GetCurThread();
	if( IsHeap(cur->ThreadName) )
		free( cur->ThreadName );
	cur->ThreadName = malloc(strlen(NewName)+1);
	strcpy(cur->ThreadName, NewName);
}

/**
 * \fn void Threads_SetTickets(int Num)
 * \brief Sets the 'priority' of a task
 */
void Threads_SetTickets(int Num)
{
	tThread	*cur = Proc_GetCurThread();
	if(Num < 0)	return;
	if(Num > MAX_TICKETS)	Num = MAX_TICKETS;
	
	LOCK( &giThreadListLock );
	giTotalTickets -= cur->NumTickets;
	cur->NumTickets = Num;
	giTotalTickets += Num;
	//LOG("giTotalTickets = %i", giTotalTickets);
	RELEASE( &giThreadListLock );
}

/**
 * \fn void Threads_WaitTID(int TID, int *status)
 * \brief Wait for a task to change state
 */
int Threads_WaitTID(int TID, int *status)
{
	Threads_Dump();
	
	// Any Child
	if(TID == -1) {
		
		return -1;
	}
	
	// Any peer/child thread
	if(TID == 0) {
		
		return -1;
	}
	
	// TGID = abs(TID)
	if(TID < -1) {
		return -1;
	}
	
	// Specific Thread
	if(TID > 0) {
		tThread	*t = Threads_GetThread(TID);
		 int	initStatus = t->Status;
		 int	ret;
		while(t->Status == initStatus)	Threads_Yield();
		ret = t->RetStatus;
		switch(t->Status)
		{
		case THREAD_STAT_ZOMBIE:
			t->Status = THREAD_STAT_DEAD;
			if(status)	*status = 0;
			Threads_AddToDelete( t );
			break;
		default:
			if(status)	*status = -1;
			break;
		}
		return ret;
	}
	
	return -1;
}

/**
 * \fn tThread *Threads_GetThread(Uint TID)
 * \brief Gets a thread given its TID
 */
tThread *Threads_GetThread(Uint TID)
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
 * \fn void Threads_AddToDelete(tThread *Thread)
 * \brief Adds a thread to the delete queue
 */
void Threads_AddToDelete(tThread *Thread)
{
	// Add to delete queue
	if(gDeleteThreads) {
		Thread->Next = gDeleteThreads;
		gDeleteThreads = Thread;
	} else {
		Thread->Next = NULL;
		gDeleteThreads = Thread;
	}
}

/**
 * \fn tThread *Threads_int_GetPrev(tThread *List, tThread *Thread)
 * \brief Gets the previous entry in a thead linked list
 */
tThread *Threads_int_GetPrev(tThread **List, tThread *Thread)
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
 * \fn void Threads_Exit(int TID, int Status)
 * \brief Exit the current process
 */
void Threads_Exit(int TID, int Status)
{
	Threads_Kill( Proc_GetCurThread(), (Uint)Status & 0xFF );
}

/**
 * \fn void Threads_Kill(tThread *Thread, int Status)
 * \brief Kill a thread
 * \param TID	Thread ID (0 for current)
 */
void Threads_Kill(tThread *Thread, int Status)
{
	tThread	*prev;
	tMsg	*msg;
	
	// Kill all children
	#if 0
	{
		tThread	*child;
		for(child = gActiveThreads;
			child;
			child = child->Next)
		{
			if(child->PTID == gCurrentThread->TID)
				Threads_Kill(child, -1);
		}
	}
	#endif
	
	///\note Double lock is needed due to overlap of locks
	
	// Lock thread (stop us recieving messages)
	LOCK( &Thread->IsLocked );
	
	// Lock thread list
	LOCK( &giThreadListLock );
	
	// Get previous thread on list
	prev = Threads_int_GetPrev( &gActiveThreads, Thread );
	if(!prev) {
		Warning("Proc_Exit - Current thread is not on the active queue");
		return;
	}
	
	// Clear Message Queue
	while( Thread->Messages )
	{
		msg = Thread->Messages->Next;
		free( Thread->Messages );
		Thread->Messages = msg;
	}
	
	Thread->Remaining = 0;	// Clear Remaining Quantum
	Thread->Quantum = 0;	// Clear Quantum to indicate dead thread
	prev->Next = Thread->Next;	// Remove from active
	
	giNumActiveThreads --;
	giTotalTickets -= Thread->NumTickets;
	
	// Mark thread as a zombie
	Thread->RetStatus = Status;
	
	// Don't Zombie if we are being killed as part of a tree
	if(Status == -1)
	{
		Thread->Status = THREAD_STAT_DEAD;
		Threads_AddToDelete( Thread );
	} else {
		Thread->Status = THREAD_STAT_ZOMBIE;
	}
	
	// Release spinlocks
	RELEASE( &Thread->IsLocked );	// Released first so that it IS released
	RELEASE( &giThreadListLock );
	if(Status != -1)	HALT();
}

/**
 * \fn void Threads_Yield()
 * \brief Yield remainder of timeslice
 */
void Threads_Yield()
{
	Proc_GetCurThread()->Remaining = 0;
	HALT();
}

/**
 * \fn void Threads_Sleep()
 * \brief Take the current process off the run queue
 */
void Threads_Sleep()
{
	tThread *cur = Proc_GetCurThread();
	tThread *thread;
	
	//Log("Proc_Sleep: %i going to sleep", gCurrentThread->TID);
	
	// Acquire Spinlock
	LOCK( &giThreadListLock );
	
	// Get thread before current thread
	thread = Threads_int_GetPrev( &gActiveThreads, cur );
	if(!thread) {
		Warning("Proc_Sleep - Current thread is not on the active queue");
		return;
	}
	
	// Don't sleep if there is a message waiting
	if( cur->Messages ) {
		RELEASE( &giThreadListLock );
		return;
	}
	
	// Unset remaining timeslices (force a task switch on timer fire)
	cur->Remaining = 0;
	
	// Remove from active list
	thread->Next = cur->Next;
	
	// Add to Sleeping List (at the top)
	cur->Next = gSleepingThreads;
	gSleepingThreads = cur;
	
	// Reduce the active count & ticket count
	giNumActiveThreads --;
	giTotalTickets -= cur->NumTickets;
	
	// Mark thread as sleeping
	cur->Status = THREAD_STAT_SLEEPING;
	
	// Release Spinlock
	RELEASE( &giThreadListLock );
	
	HALT();
}


/**
 * \fn void Threads_Wake( tThread *Thread )
 * \brief Wakes a sleeping/waiting thread up
 */
void Threads_Wake(tThread *Thread)
{
	tThread	*prev;
	switch(Thread->Status)
	{
	case THREAD_STAT_ACTIVE:	break;
	case THREAD_STAT_SLEEPING:
		LOCK( &giThreadListLock );
		prev = Threads_int_GetPrev(&gSleepingThreads, Thread);
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

#if 0
/**
 * \fn void Threads_SetSignalHandler(int Num, void *Handler)
 * \brief Sets the signal handler for a signal
 */
void Threads_SetSignalHandler(int Num, void *Handler)
{
	if(Num < 0 || Num >= NSIG)	return;
	
	gCurrentThread->SignalHandlers[Num] = Handler;
}

/**
 * \fn void Threads_SendSignal(int TID, int Num)
 */
void Threads_SendSignal(int TID, int Num)
{
	tThread	*thread = Proc_GetThread(TID);
	void	*handler;
	
	if(!thread)	return ;
	
	handler = thread->SignalHandlers[Num];
	
	// Panic?
	if(handler == SIG_ERR) {
		Proc_Kill(TID);
		return ;
	}
	// Dump Core?
	if(handler == -2) {
		Proc_Kill(TID);
		return ;
	}
	// Ignore?
	if(handler == -2)	return;
	
	// Check the type and handle if the thread is already in a signal
	if(thread->CurSignal != 0) {
		if(Num < _SIGTYPE_FATAL)
			Proc_Kill(TID);
		} else {
			while(thread->CurSignal != 0)
				Proc_Yield();
		}
	}
	
	//TODO: 
}
#endif

// --- Process Structure Access Functions ---
int Threads_GetPID()
{
	return Proc_GetCurThread()->TGID;
}
int Threads_GetTID()
{
	return Proc_GetCurThread()->TID;
}
int Threads_GetUID()
{
	return Proc_GetCurThread()->UID;
}
int Threads_GetGID()
{
	return Proc_GetCurThread()->GID;
}

/**
 * \fn void Threads_Dump()
 * \brief Dums a list of currently running threads
 */
void Threads_Dump()
{
	tThread	*thread;
	
	Log("Active Threads:");
	for(thread=gActiveThreads;thread;thread=thread->Next)
	{
		Log(" %i (%i) - %s", thread->TID, thread->TGID, thread->ThreadName);
		Log("  %i Tickets, Quantum %i", thread->NumTickets, thread->Quantum);
		Log("  KStack 0x%x", thread->KernelStack);
	}
	Log("Sleeping Threads:");
	for(thread=gSleepingThreads;thread;thread=thread->Next)
	{
		Log(" %i (%i) - %s", thread->TID, thread->TGID, thread->ThreadName);
		Log("  %i Tickets, Quantum %i", thread->NumTickets, thread->Quantum);
		Log("  KStack 0x%x", thread->KernelStack);
	}
}

/**
 * \fn tThread *Threads_GetNextToRun(int CPU)
 * \brief Gets the next thread to run
 */
tThread *Threads_GetNextToRun(int CPU)
{
	tThread	*thread;
	 int	ticket;
	 int	number;
	
	// Special case: 1 thread
	if(giNumActiveThreads == 1)
	{
		return gActiveThreads;
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
	
	return thread;
}
