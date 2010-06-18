/*
 * Acess2
 * threads.c
 * - Common Thread Control
 */
#include <acess.h>
#include <threads.h>
#include <errno.h>

// === CONSTANTS ===
#define	DEFAULT_QUANTUM	10
#define	DEFAULT_TICKETS	5
#define MAX_TICKETS		10
const enum eConfigTypes	cCONFIG_TYPES[] = {
	CFGT_HEAPSTR,	// CFG_VFS_CWD
	CFGT_INT,	// CFG_VFS_MAXFILES
	CFGT_NULL
};

// === IMPORTS ===
extern void	ArchThreads_Init(void);
extern void	Proc_Start(void);
extern tThread	*Proc_GetCurThread(void);
extern int	Proc_Clone(Uint *Err, Uint Flags);
extern void	Proc_CallFaultHandler(tThread *Thread);

// === PROTOTYPES ===
void	Threads_Init(void);
 int	Threads_SetName(char *NewName);
char	*Threads_GetName(int ID);
void	Threads_SetTickets(int Num);
tThread	*Threads_CloneTCB(Uint *Err, Uint Flags);
 int	Threads_WaitTID(int TID, int *status);
tThread	*Threads_GetThread(Uint TID);
void	Threads_AddToDelete(tThread *Thread);
tThread	*Threads_int_GetPrev(tThread **List, tThread *Thread);
void	Threads_Exit(int TID, int Status);
void	Threads_Kill(tThread *Thread, int Status);
void	Threads_Yield(void);
void	Threads_Sleep(void);
void	Threads_Wake(tThread *Thread);
void	Threads_AddActive(tThread *Thread);
 int	Threads_GetPID(void);
 int	Threads_GetTID(void);
tUID	Threads_GetUID(void);
 int	Threads_SetUID(Uint *Errno, tUID ID);
tGID	Threads_GetGID(void);
 int	Threads_SetGID(Uint *Errno, tUID ID);
void	Threads_Dump(void);

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
	
	0, 0,	// Current Fault, Fault Handler
	
	NULL, NULL,	// Messages, Last Message
	DEFAULT_QUANTUM, DEFAULT_QUANTUM,	// Quantum, Remaining
	DEFAULT_TICKETS,
	{0}	// Default config to zero
	};
// -- Processes --
// --- Locks ---
tSpinlock	glThreadListLock = 0;	///\note NEVER use a heap function while locked
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
 * \fn void Threads_Init(void)
 * \brief Initialse the thread list
 */
void Threads_Init(void)
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
		HALT();
		for(;;) {
			HALT();	// Just yeilds
		}
	}
	#endif
	
	Proc_Start();
}

/**
 * \fn void Threads_SetName(char *NewName)
 * \brief Sets the current thread's name
 */
int Threads_SetName(char *NewName)
{
	tThread	*cur = Proc_GetCurThread();
	if( IsHeap(cur->ThreadName) )
		free( cur->ThreadName );
	cur->ThreadName = malloc(strlen(NewName)+1);
	strcpy(cur->ThreadName, NewName);
	return 0;
}

/**
 * \fn char *Threads_GetName(int ID)
 * \brief Gets a thread's name
 */
char *Threads_GetName(int ID)
{
	if(ID == -1) {
		return Proc_GetCurThread()->ThreadName;
	}
	return NULL;
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
	
	LOCK( &glThreadListLock );
	giTotalTickets -= cur->NumTickets;
	cur->NumTickets = Num;
	giTotalTickets += Num;
	//LOG("giTotalTickets = %i", giTotalTickets);
	RELEASE( &glThreadListLock );
}

/**
 * \fn tThread *Threads_CloneTCB(Uint *Err, Uint Flags)
 */
tThread *Threads_CloneTCB(Uint *Err, Uint Flags)
{
	tThread	*cur, *new;
	 int	i;
	cur = Proc_GetCurThread();
	
	new = malloc(sizeof(tThread));
	if(new == NULL) {
		*Err = -ENOMEM;
		return NULL;
	}
	
	new->Next = NULL;
	new->IsLocked = 0;
	new->Status = THREAD_STAT_ACTIVE;
	new->RetStatus = 0;
	
	// Get Thread ID
	new->TID = giNextTID++;
	new->PTID = cur->TID;
	
	// Clone Name
	new->ThreadName = malloc(strlen(cur->ThreadName)+1);
	strcpy(new->ThreadName, cur->ThreadName);
	
	// Set Thread Group ID (PID)
	if(Flags & CLONE_VM)
		new->TGID = new->TID;
	else
		new->TGID = cur->TGID;
	
	// Messages are not inherited
	new->Messages = NULL;
	new->LastMessage = NULL;
	
	// Set State
	new->Remaining = new->Quantum = cur->Quantum;
	new->NumTickets = cur->NumTickets;
	
	// Set Signal Handlers
	new->CurFaultNum = 0;
	new->FaultHandler = cur->FaultHandler;
	
	for( i = 0; i < NUM_CFG_ENTRIES; i ++ )
	{
		switch(cCONFIG_TYPES[i])
		{
		default:
			new->Config[i] = cur->Config[i];
			break;
		case CFGT_HEAPSTR:
			if(cur->Config[i])
				new->Config[i] = (Uint) strdup( (void*)cur->Config[i] );
			else
				new->Config[i] = 0;
			break;
		}
	}
	
	return new;
}

/**
 * \fn Uint *Threads_GetCfgPtr(int Id)
 */
Uint *Threads_GetCfgPtr(int Id)
{
	if(Id < 0 || Id >= NUM_CFG_ENTRIES) {
		Warning("Threads_GetCfgPtr: Index %i is out of bounds", Id);
		return NULL;
	}
	
	return &Proc_GetCurThread()->Config[Id];
}

/**
 * \fn void Threads_WaitTID(int TID, int *status)
 * \brief Wait for a task to change state
 */
int Threads_WaitTID(int TID, int *status)
{	
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
		
		if(initStatus != THREAD_STAT_ZOMBIE) {
			while(t->Status == initStatus) {
				Threads_Yield();
			}
		}
		
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
 * \fn tThread *Threads_int_GetPrev(tThread **List, tThread *Thread)
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
	if( TID == 0 )
		Threads_Kill( Proc_GetCurThread(), (Uint)Status & 0xFF );
	else
		Threads_Kill( Threads_GetThread(TID), (Uint)Status & 0xFF );
	for(;;)	HALT();	// Just in case
}

/**
 * \fn void Threads_Kill(tThread *Thread, int Status)
 * \brief Kill a thread
 * \param Thread	Thread to kill
 * \param Status	Status code to return to the parent
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
			if(child->PTID == Thread->TID)
				Threads_Kill(child, -1);
		}
	}
	#endif
	
	///\note Double lock is needed due to overlap of locks
	
	// Lock thread (stop us recieving messages)
	LOCK( &Thread->IsLocked );
	
	// Lock thread list
	LOCK( &glThreadListLock );
	
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
	RELEASE( &glThreadListLock );
	
	//Log("Thread %i went *hurk*", Thread->TID);
	
	if(Status != -1)	HALT();
}

/**
 * \fn void Threads_Yield(void)
 * \brief Yield remainder of timeslice
 */
void Threads_Yield(void)
{
	Proc_GetCurThread()->Remaining = 0;
	HALT();
}

/**
 * \fn void Threads_Sleep(void)
 * \brief Take the current process off the run queue
 */
void Threads_Sleep(void)
{
	tThread *cur = Proc_GetCurThread();
	tThread *thread;
	
	//Log_Log("Threads", "%i going to sleep", cur->TID);
	
	// Acquire Spinlock
	LOCK( &glThreadListLock );
	
	// Get thread before current thread
	thread = Threads_int_GetPrev( &gActiveThreads, cur );
	if(!thread) {
		Warning("Threads_Sleep - Current thread is not on the active queue");
		Threads_Dump();
		return;
	}
	
	// Don't sleep if there is a message waiting
	if( cur->Messages ) {
		RELEASE( &glThreadListLock );
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
	RELEASE( &glThreadListLock );
	
	while(cur->Status != THREAD_STAT_ACTIVE)	HALT();
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
		//Log_Log("Threads", "Waking %i (%p) from sleeping", Thread->TID, Thread);
		LOCK( &glThreadListLock );
		prev = Threads_int_GetPrev(&gSleepingThreads, Thread);
		prev->Next = Thread->Next;	// Remove from sleeping queue
		Thread->Next = gActiveThreads;	// Add to active queue
		gActiveThreads = Thread;
		giNumActiveThreads ++;
		giTotalTickets += Thread->NumTickets;
		Thread->Status = THREAD_STAT_ACTIVE;
		RELEASE( &glThreadListLock );
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

void Threads_WakeTID(tTID Thread)
{
	Threads_Wake( Threads_GetThread(Thread) );
}

/**
 * \fn void Threads_AddActive(tThread *Thread)
 * \brief Adds a thread to the active queue
 */
void Threads_AddActive(tThread *Thread)
{
	LOCK( &glThreadListLock );
	Thread->Next = gActiveThreads;
	gActiveThreads = Thread;
	giNumActiveThreads ++;
	giTotalTickets += Thread->NumTickets;
	//Log("Threads_AddActive: giNumActiveThreads = %i, giTotalTickets = %i",
	//	giNumActiveThreads, giTotalTickets);
	RELEASE( &glThreadListLock );
}

/**
 * \fn void Threads_SetFaultHandler(Uint Handler)
 * \brief Sets the signal handler for a signal
 */
void Threads_SetFaultHandler(Uint Handler)
{	
	Log_Log("Threads", "Threads_SetFaultHandler: Handler = %p", Handler);
	Proc_GetCurThread()->FaultHandler = Handler;
}

/**
 * \fn void Threads_Fault(int Num)
 * \brief Calls a fault handler
 */
void Threads_Fault(int Num)
{
	tThread	*thread = Proc_GetCurThread();
	
	Log_Log("Threads", "Threads_Fault: thread = %p", thread);
	
	if(!thread)	return ;
	
	Log_Log("Threads", "Threads_Fault: thread->FaultHandler = %p", thread->FaultHandler);
	
	switch(thread->FaultHandler)
	{
	case 0:	// Panic?
		Threads_Kill(thread, -1);
		HALT();
		return ;
	case 1:	// Dump Core?
		Threads_Kill(thread, -1);
		HALT();
		return ;
	}
	
	// Double Fault? Oh, F**k
	if(thread->CurFaultNum != 0) {
		Threads_Kill(thread, -1);	// For now, just kill
		HALT();
	}
	
	thread->CurFaultNum = Num;
	
	Proc_CallFaultHandler(thread);
}

// --- Process Structure Access Functions ---
tPID Threads_GetPID(void)
{
	return Proc_GetCurThread()->TGID;
}
tTID Threads_GetTID(void)
{
	return Proc_GetCurThread()->TID;
}
tUID Threads_GetUID(void)
{
	return Proc_GetCurThread()->UID;
}
tGID Threads_GetGID(void)
{
	return Proc_GetCurThread()->GID;
}

int Threads_SetUID(Uint *Errno, tUID ID)
{
	tThread	*t = Proc_GetCurThread();
	if( t->UID != 0 ) {
		*Errno = -EACCES;
		return -1;
	}
	Log("Threads_SetUID - Setting User ID to %i", ID);
	t->UID = ID;
	return 0;
}

int Threads_SetGID(Uint *Errno, tGID ID)
{
	tThread	*t = Proc_GetCurThread();
	if( t->UID != 0 ) {
		*Errno = -EACCES;
		return -1;
	}
	Log("Threads_SetGID - Setting Group ID to %i", ID);
	t->GID = ID;
	return 0;
}

/**
 * \fn void Threads_Dump(void)
 * \brief Dums a list of currently running threads
 */
void Threads_Dump(void)
{
	tThread	*thread;
	tThread	*cur = Proc_GetCurThread();
	
	Log("Active Threads:");
	for(thread=gActiveThreads;thread;thread=thread->Next)
	{
		Log("%c%i (%i) - %s",
			(thread==cur?'*':' '),
			thread->TID, thread->TGID, thread->ThreadName);
		Log("  %i Tickets, Quantum %i", thread->NumTickets, thread->Quantum);
		Log("  KStack 0x%x", thread->KernelStack);
	}
	Log("Sleeping Threads:");
	for(thread=gSleepingThreads;thread;thread=thread->Next)
	{
		Log("%c%i (%i) - %s",
			(thread==cur?'*':' '),
			thread->TID, thread->TGID, thread->ThreadName);
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
	
	if(giNumActiveThreads == 0) {
		return NULL;
	}
	
	// Special case: 1 thread
	if(giNumActiveThreads == 1) {
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

/**
 * \fn void Threads_SegFault(tVAddr Addr)
 * \brief Called when a Segment Fault occurs
 */
void Threads_SegFault(tVAddr Addr)
{
	Warning("Thread #%i committed a segfault at address %p", Proc_GetCurThread()->TID, Addr);
	Threads_Fault( 1 );
	//Threads_Exit( 0, -1 );
}

// === EXPORTS ===
EXPORT(Threads_GetUID);
