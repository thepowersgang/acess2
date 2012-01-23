/*
 * Acess2
 * threads.c
 * - Common Thread Control
 */
#include <acess.h>
#include <threads.h>
#include <threads_int.h>
#include <errno.h>
#include <hal_proc.h>
#include <semaphore.h>

// Configuration
#define DEBUG_TRACE_TICKETS	0	// Trace ticket counts
#define DEBUG_TRACE_STATE	0	// Trace state changes (sleep/wake)

// --- Schedulers ---
#define SCHED_UNDEF	0
#define SCHED_LOTTERY	1	// Lottery scheduler
#define SCHED_RR_SIM	2	// Single Queue Round Robin
#define SCHED_RR_PRI	3	// Multi Queue Round Robin
// Set scheduler type
#define SCHEDULER_TYPE	SCHED_RR_PRI

// === CONSTANTS ===
#define	DEFAULT_QUANTUM	5
#define	DEFAULT_PRIORITY	5
#define MIN_PRIORITY		10
const enum eConfigTypes	cCONFIG_TYPES[NUM_CFG_ENTRIES] = {
	CFGT_HEAPSTR,	// e.g. CFG_VFS_CWD
	CFGT_INT,	// e.g. CFG_VFS_MAXFILES
	CFGT_NULL
};

// === IMPORTS ===

// === TYPE ===
typedef struct
{
	tThread	*Head;
	tThread	*Tail;
} tThreadList;

// === PROTOTYPES ===
void	Threads_Init(void);
#if 0
void	Threads_Delete(tThread *Thread);
 int	Threads_SetName(const char *NewName);
#endif
char	*Threads_GetName(int ID);
#if 0
void	Threads_SetPriority(tThread *Thread, int Pri);
tThread	*Threads_CloneTCB(Uint *Err, Uint Flags);
 int	Threads_WaitTID(int TID, int *status);
tThread	*Threads_GetThread(Uint TID);
#endif
tThread	*Threads_int_DelFromQueue(tThreadList *List, tThread *Thread);
void	Threads_int_AddToList(tThreadList *List, tThread *Thread);
#if 0
void	Threads_Exit(int TID, int Status);
void	Threads_Kill(tThread *Thread, int Status);
void	Threads_Yield(void);
void	Threads_Sleep(void);
 int	Threads_Wake(tThread *Thread);
void	Threads_AddActive(tThread *Thread);
tThread	*Threads_RemActive(void);
#endif
void	Threads_ToggleTrace(int TID);
void	Threads_Fault(int Num);
void	Threads_SegFault(tVAddr Addr);
#if 0
 int	Threads_GetPID(void);
 int	Threads_GetTID(void);
tUID	Threads_GetUID(void);
tGID	Threads_GetGID(void);
 int	Threads_SetUID(Uint *Errno, tUID ID);
 int	Threads_SetGID(Uint *Errno, tUID ID);
#endif
void	Threads_Dump(void);
void	Threads_DumpActive(void);

// === GLOBALS ===
// -- Core Thread --
// Only used for the core kernel
tThread	gThreadZero = {
	.Status 	= THREAD_STAT_ACTIVE,	// Status
	.ThreadName	= (char*)"ThreadZero",	// Name
	.Quantum	= DEFAULT_QUANTUM,	// Default Quantum
	.Remaining	= DEFAULT_QUANTUM,	// Current Quantum
	.Priority	= DEFAULT_PRIORITY	// Number of tickets
	};
// -- Processes --
// --- Locks ---
tShortSpinlock	glThreadListLock;	///\note NEVER use a heap function while locked
// --- Current State ---
volatile int	giNumActiveThreads = 0;	// Number of threads on the active queue
volatile Uint	giNextTID = 1;	// Next TID to allocate
// --- Thread Lists ---
tThread	*gAllThreads = NULL;		// All allocated threads
tThreadList	gSleepingThreads;	// Sleeping Threads
 int	giNumCPUs = 1;	// Number of CPUs
BOOL     gaThreads_NoTaskSwitch[MAX_CPUS];	// Disables task switches for each core (Pseudo-IF)
// --- Scheduler Types ---
#if SCHEDULER_TYPE == SCHED_LOTTERY
const int	caiTICKET_COUNTS[MIN_PRIORITY+1] = {100,81,64,49,36,25,16,9,4,1,0};
volatile int	giFreeTickets = 0;	// Number of tickets held by non-scheduled threads
tThreadList	gActiveThreads;		// Currently Running Threads
#elif SCHEDULER_TYPE == SCHED_RR_SIM
tThreadList	gActiveThreads;		// Currently Running Threads
#elif SCHEDULER_TYPE == SCHED_RR_PRI
tThreadList	gaActiveThreads[MIN_PRIORITY+1];	// Active threads for each priority level
#else
# error "Unkown scheduler type"
#endif

// === CODE ===
/**
 * \fn void Threads_Init(void)
 * \brief Initialse the thread list
 */
void Threads_Init(void)
{
	ArchThreads_Init();
	
	Log_Debug("Threads", "Offsets of tThread");
	Log_Debug("Threads", ".Priority = %i", offsetof(tThread, Priority));
	Log_Debug("Threads", ".KernelStack = %i", offsetof(tThread, KernelStack));
	
	// Create Initial Task
//	#if SCHEDULER_TYPE == SCHED_RR_PRI
//	gaActiveThreads[gThreadZero.Priority].Head = &gThreadZero;
//	gaActiveThreads[gThreadZero.Priority].Tail = &gThreadZero;
//	#else
//	gActiveThreads.Head = &gThreadZero;
//	gActiveThreads.Tail = &gThreadZero;
//	#endif
	
	gAllThreads = &gThreadZero;
	giNumActiveThreads = 1;
		
	Proc_Start();
}

void Threads_Delete(tThread *Thread)
{
	// Set to dead
	Thread->Status = THREAD_STAT_BURIED;

	// Clear out process state
	Proc_ClearThread(Thread);			
	
	// Free name
	if( IsHeap(Thread->ThreadName) )
		free(Thread->ThreadName);
	
	// Remove from global list
	// TODO: Lock this too
	if( Thread == gAllThreads )
		gAllThreads = Thread->GlobalNext;
	else
		Thread->GlobalPrev->GlobalNext = Thread->GlobalNext;
	
	free(Thread);
}

/**
 * \fn void Threads_SetName(const char *NewName)
 * \brief Sets the current thread's name
 * \param NewName	New name for the thread
 * \return Boolean Failure
 */
int Threads_SetName(const char *NewName)
{
	tThread	*cur = Proc_GetCurThread();
	char	*oldname = cur->ThreadName;
	
	// NOTE: There is a possibility of non-thread safety here
	// A thread could read the current name pointer before it is zeroed
	
	cur->ThreadName = NULL;
	
	if( IsHeap(oldname) )	free( oldname );
	
	cur->ThreadName = strdup(NewName);
	return 0;
}

/**
 * \fn char *Threads_GetName(int ID)
 * \brief Gets a thread's name
 * \param ID	Thread ID (-1 indicates current thread)
 * \return Pointer to name
 * \retval NULL	Failure
 */
char *Threads_GetName(tTID ID)
{
	if(ID == -1) {
		return Proc_GetCurThread()->ThreadName;
	}
	return Threads_GetThread(ID)->ThreadName;
}

/**
 * \fn void Threads_SetPriority(tThread *Thread, int Pri)
 * \brief Sets the priority of a task
 * \param Thread	Thread to update ticket count (NULL means current thread)
 * \param Pri	New priority
 */
void Threads_SetPriority(tThread *Thread, int Pri)
{
	// Get current thread
	if(Thread == NULL)	Thread = Proc_GetCurThread();
	// Bounds checking
	// - If < 0, set to lowest priority
	// - Minumum priority is actualy a high number, 0 is highest
	if(Pri < 0)	Pri = MIN_PRIORITY;
	if(Pri > MIN_PRIORITY)	Pri = MIN_PRIORITY;
	
	// Do we actually have to do anything?
	if( Pri == Thread->Priority )	return;
	
	#if SCHEDULER_TYPE == SCHED_RR_PRI
	if( Thread != Proc_GetCurThread() )
	{
		SHORTLOCK( &glThreadListLock );
		// Remove from old priority
		Threads_int_DelFromQueue( &gaActiveThreads[Thread->Priority], Thread );
		// And add to new
		Threads_int_AddToList( &gaActiveThreads[Pri], Thread );
		Thread->Priority = Pri;
		SHORTREL( &glThreadListLock );
	}
	else
		Thread->Priority = Pri;
	#else
	// If this isn't the current thread, we need to lock
	if( Thread != Proc_GetCurThread() )
	{
		SHORTLOCK( &glThreadListLock );
		
		#if SCHEDULER_TYPE == SCHED_LOTTERY
		giFreeTickets -= caiTICKET_COUNTS[Thread->Priority] - caiTICKET_COUNTS[Pri];
		# if DEBUG_TRACE_TICKETS
		Log("Threads_SetTickets: new giFreeTickets = %i [-%i+%i]",
			giFreeTickets,
			caiTICKET_COUNTS[Thread->Priority], caiTICKET_COUNTS[Pri]);
		# endif
		#endif
		Thread->Priority = Pri;
		SHORTREL( &glThreadListLock );
	}
	else
		Thread->Priority = Pri;
	#endif
	
	#if DEBUG_TRACE_STATE
	Log("Threads_SetPriority: %p(%i %s) pri set %i",
		Thread, Thread->TID, Thread->ThreadName,
		Pri);
	#endif
}

/**
 * \brief Clone the TCB of the current thread
 * \param Flags	Flags for something... (What is this for?)
 */
tThread *Threads_CloneTCB(Uint Flags)
{
	tThread	*cur, *new;
	 int	i;
	cur = Proc_GetCurThread();
	
	// Allocate and duplicate
	new = malloc(sizeof(tThread));
	if(new == NULL) { errno = -ENOMEM; return NULL; }
	memcpy(new, cur, sizeof(tThread));
	
	new->CurCPU = -1;
	new->Next = NULL;
	memset( &new->IsLocked, 0, sizeof(new->IsLocked));
	new->Status = THREAD_STAT_PREINIT;
	new->RetStatus = 0;
	
	// Get Thread ID
	new->TID = giNextTID++;
	new->Parent = cur;
	new->bInstrTrace = 0;
	
	// Clone Name
	new->ThreadName = strdup(cur->ThreadName);
	
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
	new->Priority = cur->Priority;
	
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
	
	// Maintain a global list of threads
	SHORTLOCK( &glThreadListLock );
	new->GlobalPrev = NULL;	// Protect against bugs
	new->GlobalNext = gAllThreads;
	gAllThreads->GlobalPrev = new;
	gAllThreads = new;
	SHORTREL( &glThreadListLock );
	
	return new;
}

/**
 * \brief Clone the TCB of the kernel thread
 */
tThread *Threads_CloneThreadZero(void)
{
	tThread	*new;
	 int	i;
	
	// Allocate and duplicate
	new = malloc(sizeof(tThread));
	if(new == NULL) {
		return NULL;
	}
	memcpy(new, &gThreadZero, sizeof(tThread));
	
	new->CurCPU = -1;
	new->Next = NULL;
	memset( &new->IsLocked, 0, sizeof(new->IsLocked));
	new->Status = THREAD_STAT_PREINIT;
	new->RetStatus = 0;
	
	// Get Thread ID
	new->TID = giNextTID++;
	new->Parent = 0;
	
	// Clone Name
	new->ThreadName = NULL;
	
	// Messages are not inherited
	new->Messages = NULL;
	new->LastMessage = NULL;
	
	// Set State
	new->Remaining = new->Quantum = DEFAULT_QUANTUM;
	new->Priority = DEFAULT_PRIORITY;
	new->bInstrTrace = 0;
	
	// Set Signal Handlers
	new->CurFaultNum = 0;
	new->FaultHandler = 0;
	
	for( i = 0; i < NUM_CFG_ENTRIES; i ++ )
	{
		new->Config[i] = 0;
	}
	
	// Maintain a global list of threads
	SHORTLOCK( &glThreadListLock );
	new->GlobalPrev = NULL;	// Protect against bugs
	new->GlobalNext = gAllThreads;
	gAllThreads->GlobalPrev = new;
	gAllThreads = new;
	SHORTREL( &glThreadListLock );
	
	return new;
}

/**
 * \brief Get a configuration pointer from the Per-Thread data area
 * \param ID	Config slot ID
 * \return Pointer at ID
 */
Uint *Threads_GetCfgPtr(int ID)
{
	if(ID < 0 || ID >= NUM_CFG_ENTRIES) {
		Warning("Threads_GetCfgPtr: Index %i is out of bounds", ID);
		return NULL;
	}
	
	return &Proc_GetCurThread()->Config[ID];
}

/**
 * \brief Wait for a task to change state
 * \param TID	Thread ID to wait on (-1: Any child thread, 0: Any Child/Sibling, <-1: -PID)
 * \param Status	Thread return status
 * \return TID of child that changed state
 */
tTID Threads_WaitTID(int TID, int *Status)
{	
	// Any Child
	if(TID == -1) {
		Log_Error("Threads", "TODO: Threads_WaitTID(TID=-1) - Any Child");
		return -1;
	}
	
	// Any peer/child thread
	if(TID == 0) {
		Log_Error("Threads", "TODO: Threads_WaitTID(TID=0) - Any Child/Sibling");
		return -1;
	}
	
	// TGID = abs(TID)
	if(TID < -1) {
		Log_Error("Threads", "TODO: Threads_WaitTID(TID<0) - TGID");
		return -1;
	}
	
	// Specific Thread
	if(TID > 0) {
		tThread	*t = Threads_GetThread(TID);
		tTID	ret;
		
		// Wait for the thread to die!
		// TODO: Handle child also being suspended if wanted
		while(t->Status != THREAD_STAT_ZOMBIE) {
			Threads_Sleep();
			Log_Debug("Threads", "%i waiting for %i, t->Status = %i",
				Threads_GetTID(), t->TID, t->Status);
		}
		
		// Set return status
		ret = t->TID;
		switch(t->Status)
		{
		case THREAD_STAT_ZOMBIE:
			// Kill the thread
			t->Status = THREAD_STAT_DEAD;
			// TODO: Child return value?
			if(Status)	*Status = t->RetStatus;
			// add to delete queue
			Threads_Delete( t );
			break;
		default:
			if(Status)	*Status = -1;
			break;
		}
		return ret;
	}
	
	return -1;
}

/**
 * \brief Gets a thread given its TID
 * \param TID	Thread ID
 * \return Thread pointer
 */
tThread *Threads_GetThread(Uint TID)
{
	tThread *thread;
	
	// Search global list
	for(thread = gAllThreads;
		thread;
		thread = thread->GlobalNext)
	{
		if(thread->TID == TID)
			return thread;
	}

	Log("Unable to find TID %i on main list\n", TID);
	
	return NULL;
}

/**
 * \brief Deletes an entry from a list
 * \param List	Pointer to the list head
 * \param Thread	Thread to find
 * \return \a Thread
 */
tThread *Threads_int_DelFromQueue(tThreadList *List, tThread *Thread)
{
	tThread *ret, *prev = NULL;
	
	for(ret = List->Head;
		ret && ret != Thread;
		prev = ret, ret = ret->Next
		);
	
	// Is the thread on the list
	if(!ret) {
		//LogF("%p(%s) is not on list %p\n", Thread, Thread->ThreadName, List);
		return NULL;
	}
	
	if( !prev ) {
		List->Head = Thread->Next;
		//LogF("%p(%s) removed from head of %p\n", Thread, Thread->ThreadName, List);
	}
	else {
		prev->Next = Thread->Next;
		//LogF("%p(%s) removed from %p (prev=%p)\n", Thread, Thread->ThreadName, List, prev);
	}
	if( Thread->Next == NULL )
		List->Tail = prev;
	
	return Thread;
}

void Threads_int_AddToList(tThreadList *List, tThread *Thread)
{
	if( List->Head )
		List->Tail->Next = Thread;
	else
		List->Head = Thread;
	List->Tail = Thread;
	Thread->Next = NULL;
}

/**
 * \brief Exit the current process (or another?)
 * \param TID	Thread ID to kill
 * \param Status	Exit status
 */
void Threads_Exit(int TID, int Status)
{
	if( TID == 0 )
		Threads_Kill( Proc_GetCurThread(), (Uint)Status & 0xFF );
	else
		Threads_Kill( Threads_GetThread(TID), (Uint)Status & 0xFF );
	
	// Halt forever, just in case
	for(;;)	HALT();
}

/**
 * \fn void Threads_Kill(tThread *Thread, int Status)
 * \brief Kill a thread
 * \param Thread	Thread to kill
 * \param Status	Status code to return to the parent
 */
void Threads_Kill(tThread *Thread, int Status)
{
	tMsg	*msg;
	 int	isCurThread = Thread == Proc_GetCurThread();
	
	// TODO: Disown all children?
	#if 1
	{
		tThread	*child;
		// TODO: I should keep a .Children list
		for(child = gAllThreads;
			child;
			child = child->GlobalNext)
		{
			if(child->Parent == Thread)
				child->Parent = &gThreadZero;
		}
	}
	#endif
	
	///\note Double lock is needed due to overlap of lock areas
	
	// Lock thread (stop us recieving messages)
	SHORTLOCK( &Thread->IsLocked );
	
	// Clear Message Queue
	while( Thread->Messages )
	{
		msg = Thread->Messages->Next;
		free( Thread->Messages );
		Thread->Messages = msg;
	}
	
	// Lock thread list
	SHORTLOCK( &glThreadListLock );
	
	switch(Thread->Status)
	{
	case THREAD_STAT_PREINIT:	// Only on main list
		break;
	
	// Currently active thread
	case THREAD_STAT_ACTIVE:
		if( Thread != Proc_GetCurThread() )
		{
			#if SCHEDULER_TYPE == SCHED_RR_PRI
			tThreadList	*list = &gaActiveThreads[Thread->Priority];
			#else
			tThreadList	*list = &gActiveThreads;
			#endif
			if( Threads_int_DelFromQueue( list, Thread ) )
			{
			}
			else
			{
				Log_Warning("Threads",
					"Threads_Kill - Thread %p(%i,%s) marked as active, but not on list",
					Thread, Thread->TID, Thread->ThreadName
					);
			}
			#if SCHEDULER_TYPE == SCHED_LOTTERY
			giFreeTickets -= caiTICKET_COUNTS[ Thread->Priority ];
			#endif
		}
		// Ensure that we are not rescheduled
		Thread->Remaining = 0;	// Clear Remaining Quantum
		Thread->Quantum = 0;	// Clear Quantum to indicate dead thread
			
		// Update bookkeeping
		giNumActiveThreads --;
		break;
	// Kill it while it sleeps!
	case THREAD_STAT_SLEEPING:
		if( !Threads_int_DelFromQueue( &gSleepingThreads, Thread ) )
		{
			Log_Warning("Threads",
				"Threads_Kill - Thread %p(%i,%s) marked as sleeping, but not on list",
				Thread, Thread->TID, Thread->ThreadName
				);
		}
		break;
	
	// Brains!... You cannot kill something that is already dead
	case THREAD_STAT_ZOMBIE:
		Log_Warning("Threads", "Threads_Kill - Thread %p(%i,%s) is undead, you cannot kill it",
			Thread, Thread->TID, Thread->ThreadName);
		SHORTREL( &glThreadListLock );
		SHORTREL( &Thread->IsLocked );
		return ;
	
	default:
		Log_Warning("Threads", "Threads_Kill - BUG Un-checked status (%i)",
			Thread->Status);
		break;
	}
	
	// Save exit status
	Thread->RetStatus = Status;

	SHORTREL( &Thread->IsLocked );

	Thread->Status = THREAD_STAT_ZOMBIE;
	SHORTREL( &glThreadListLock );
	// TODO: Send something like SIGCHLD
	Threads_Wake( Thread->Parent );
	
	Log("Thread %i went *hurk* (%i)", Thread->TID, Status);
	
	// And, reschedule
	if(isCurThread)
	{
		for( ;; )
			Proc_Reschedule();
	}
}

/**
 * \brief Yield remainder of the current thread's timeslice
 */
void Threads_Yield(void)
{
//	Log("Threads_Yield: by %p", __builtin_return_address(0));
	Proc_Reschedule();
}

/**
 * \fn void Threads_Sleep(void)
 * \brief Take the current process off the run queue
 */
void Threads_Sleep(void)
{
	tThread *cur = Proc_GetCurThread();
	
	// Acquire Spinlock
	SHORTLOCK( &glThreadListLock );
	
	// Don't sleep if there is a message waiting
	if( cur->Messages ) {
		SHORTREL( &glThreadListLock );
		return;
	}
	
	// Remove us from running queue
	Threads_RemActive();
	// Mark thread as sleeping
	cur->Status = THREAD_STAT_SLEEPING;
	
	// Add to Sleeping List (at the top)
	Threads_int_AddToList( &gSleepingThreads, cur );
	
	#if DEBUG_TRACE_STATE
	Log("Threads_Sleep: %p (%i %s) sleeping", cur, cur->TID, cur->ThreadName);
	#endif
	
	// Release Spinlock
	SHORTREL( &glThreadListLock );

	while(cur->Status != THREAD_STAT_ACTIVE) {
		Proc_Reschedule();
		if( cur->Status != THREAD_STAT_ACTIVE )
			Log("%i - Huh? why am I up? zzzz...", cur->TID);
	}
}


/**
 * \brief Wakes a sleeping/waiting thread up
 * \param Thread	Thread to wake
 * \return Boolean Failure (Returns ERRNO)
 * \warning This should ONLY be called with task switches disabled
 */
int Threads_Wake(tThread *Thread)
{
	if(!Thread)
		return -EINVAL;
	
	switch(Thread->Status)
	{
	case THREAD_STAT_ACTIVE:
		Log("Threads_Wake - Waking awake thread (%i)", Thread->TID);
		return -EALREADY;
	
	case THREAD_STAT_SLEEPING:
		SHORTLOCK( &glThreadListLock );
		// Remove from sleeping queue
		Threads_int_DelFromQueue(&gSleepingThreads, Thread);
		
		SHORTREL( &glThreadListLock );
		Threads_AddActive( Thread );
		
		#if DEBUG_TRACE_STATE
		Log("Threads_Sleep: %p (%i %s) woken", Thread, Thread->TID, Thread->ThreadName);
		#endif
		return -EOK;
	
	case THREAD_STAT_SEMAPHORESLEEP: {
		tSemaphore	*sem;
		tThread	*th, *prev=NULL;
		
		sem = Thread->WaitPointer;
		
		SHORTLOCK( &sem->Protector );
		
		// Remove from sleeping queue
		for( th = sem->Waiting; th; prev = th, th = th->Next )
			if( th == Thread )	break;
		if( th )
		{
			if(prev)
				prev->Next = Thread->Next;
			else
				sem->Waiting = Thread->Next;
			if(sem->LastWaiting == Thread)
				sem->LastWaiting = prev;
		}
		else
		{
			prev = NULL;
			for( th = sem->Signaling; th; prev = th, th = th->Next )
				if( th == Thread )	break;
			if( !th ) {
				Log_Warning("Threads", "Thread %p(%i %s) is not on semaphore %p(%s:%s)",
					Thread, Thread->TID, Thread->ThreadName,
					sem, sem->ModName, sem->Name);
				return -EINTERNAL;
			}
			
			if(prev)
				prev->Next = Thread->Next;
			else
				sem->Signaling = Thread->Next;
			if(sem->LastSignaling == Thread)
				sem->LastSignaling = prev;
		}
		
		Thread->RetStatus = 0;	// It didn't get anything
		Threads_AddActive( Thread );
		
		#if DEBUG_TRACE_STATE
		Log("Threads_Sleep: %p(%i %s) woken from semaphore", Thread, Thread->TID, Thread->ThreadName);
		#endif
		SHORTREL( &sem->Protector );
		} return -EOK;
	
	case THREAD_STAT_WAITING:
		Warning("Threads_Wake - Waiting threads are not currently supported");
		return -ENOTIMPL;
	
	case THREAD_STAT_DEAD:
		Warning("Threads_Wake - Attempt to wake dead thread (%i)", Thread->TID);
		return -ENOTIMPL;
	
	default:
		Warning("Threads_Wake - Unknown process status (%i)\n", Thread->Status);
		return -EINTERNAL;
	}
}

/**
 * \brief Wake a thread given the TID
 * \param TID	Thread ID to wake
 * \return Boolean Faulure (errno)
 */
int Threads_WakeTID(tTID TID)
{
	tThread	*thread = Threads_GetThread(TID);
	 int	ret;
	if(!thread)
		return -ENOENT;
	ret = Threads_Wake( thread );
	//Log_Debug("Threads", "TID %i woke %i (%p)", Threads_GetTID(), TID, thread);
	return ret;
}

void Threads_ToggleTrace(int TID)
{
	tThread	*thread = Threads_GetThread(TID);
	if(!thread)	return ;
	thread->bInstrTrace = !thread->bInstrTrace;
}

/**
 * \brief Adds a thread to the active queue
 */
void Threads_AddActive(tThread *Thread)
{
	SHORTLOCK( &glThreadListLock );
	
	if( Thread->Status == THREAD_STAT_ACTIVE ) {
		tThread	*cur = Proc_GetCurThread();
		Log_Warning("Threads", "WTF, %p CPU%i %p (%i %s) is adding %p (%i %s) when it is active",
			__builtin_return_address(0),
			GetCPUNum(), cur, cur->TID, cur->ThreadName, Thread, Thread->TID, Thread->ThreadName);
		SHORTREL( &glThreadListLock );
		return ;
	}
	
	// Set state
	Thread->Status = THREAD_STAT_ACTIVE;
//	Thread->CurCPU = -1;
	// Add to active list
	{
		#if SCHEDULER_TYPE == SCHED_RR_PRI
		tThreadList	*list = &gaActiveThreads[Thread->Priority];
		#else
		tThreadList	*list = &gActiveThreads;
		#endif
		Threads_int_AddToList( list, Thread );
	}
	
	// Update bookkeeping
	giNumActiveThreads ++;
	
	#if SCHEDULER_TYPE == SCHED_LOTTERY
	{
		 int	delta;
		// Only change the ticket count if the thread is un-scheduled
		if(Thread->CurCPU != -1)
			delta = 0;
		else
			delta = caiTICKET_COUNTS[ Thread->Priority ];
		
		giFreeTickets += delta;
		# if DEBUG_TRACE_TICKETS
		Log("CPU%i %p (%i %s) added, new giFreeTickets = %i [+%i]",
			GetCPUNum(), Thread, Thread->TID, Thread->ThreadName,
			giFreeTickets, delta
			);
		# endif
	}
	#endif
	
	SHORTREL( &glThreadListLock );
}

/**
 * \brief Removes the current thread from the active queue
 * \warning This should ONLY be called with the lock held
 * \return Current thread pointer
 */
tThread *Threads_RemActive(void)
{
	#if 0
	tThread	*ret = Proc_GetCurThread();

	if( !IS_LOCKED(&glThreadListLock) ) {
		Log_KernelPanic("Threads", "Threads_RemActive called without lock held");
		return NULL;
	}
	
	// Delete from active queue
	#if SCHEDULER_TYPE == SCHED_RR_PRI
	if( !Threads_int_DelFromQueue(&gaActiveThreads[ret->Priority], ret) )
	#else
	if( !Threads_int_DelFromQueue(&gActiveThreads, ret) )
	#endif
	{
		Log_Warning("Threads", "Current thread %p(%i %s) is not on active queue",
			ret, ret->TID, ret->ThreadName
			);
		return NULL;
	}
	
	ret->Next = NULL;
	ret->Remaining = 0;
	
	giNumActiveThreads --;
	// no need to decrement tickets, scheduler did it for us
	
	#if SCHEDULER_TYPE == SCHED_LOTTERY && DEBUG_TRACE_TICKETS
	Log("CPU%i %p (%i %s) removed, giFreeTickets = %i [nc]",
		GetCPUNum(), ret, ret->TID, ret->ThreadName, giFreeTickets);
	#endif
	
	return ret;
	#else
	return Proc_GetCurThread();
	#endif
}

/**
 * \fn void Threads_SetFaultHandler(Uint Handler)
 * \brief Sets the signal handler for a signal
 */
void Threads_SetFaultHandler(Uint Handler)
{	
	//Log_Debug("Threads", "Threads_SetFaultHandler: Handler = %p", Handler);
	Proc_GetCurThread()->FaultHandler = Handler;
}

/**
 * \fn void Threads_Fault(int Num)
 * \brief Calls a fault handler
 */
void Threads_Fault(int Num)
{
	tThread	*thread = Proc_GetCurThread();
	
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
		Log_Warning("Threads", "Threads_Fault: Double fault on %i", thread->TID);
		Threads_Kill(thread, -1);	// For now, just kill
		HALT();
	}
	
	thread->CurFaultNum = Num;
	
	Proc_CallFaultHandler(thread);
}

/**
 * \fn void Threads_SegFault(tVAddr Addr)
 * \brief Called when a Segment Fault occurs
 */
void Threads_SegFault(tVAddr Addr)
{
	tThread	*cur = Proc_GetCurThread();
	cur->bInstrTrace = 0;
	Log_Warning("Threads", "Thread #%i committed a segfault at address %p", cur->TID, Addr);
	MM_DumpTables(0, USER_MAX);
	Threads_Fault( 1 );
	//Threads_Exit( 0, -1 );
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
	Log_Debug("Threads", "TID %i's UID set to %i", t->TID, ID);
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
	Log_Debug("Threads", "TID %i's GID set to %i", t->TID, ID);
	t->GID = ID;
	return 0;
}

/**
 * \fn void Threads_Dump(void)
 */
void Threads_DumpActive(void)
{
	tThread	*thread;
	tThreadList	*list;
	#if SCHEDULER_TYPE == SCHED_RR_PRI
	 int	i;
	#endif
	
	Log("Active Threads: (%i reported)", giNumActiveThreads);
	
	#if SCHEDULER_TYPE == SCHED_RR_PRI
	for( i = 0; i < MIN_PRIORITY+1; i++ )
	{
		list = &gaActiveThreads[i];
	#else
		list = &gActiveThreads;
	#endif
		for(thread=list->Head;thread;thread=thread->Next)
		{
			Log(" %p %i (%i) - %s (CPU %i)",
				thread, thread->TID, thread->TGID, thread->ThreadName, thread->CurCPU);
			if(thread->Status != THREAD_STAT_ACTIVE)
				Log("  ERROR State (%i) != THREAD_STAT_ACTIVE (%i)",
					thread->Status, THREAD_STAT_ACTIVE);
			Log("  Priority %i, Quantum %i", thread->Priority, thread->Quantum);
			Log("  KStack 0x%x", thread->KernelStack);
			if( thread->bInstrTrace )
				Log("  Tracing Enabled");
			Proc_DumpThreadCPUState(thread);
		}
	
	#if SCHEDULER_TYPE == SCHED_RR_PRI
	}
	#endif
}

/**
 * \fn void Threads_Dump(void)
 * \brief Dumps a list of currently running threads
 */
void Threads_Dump(void)
{
	tThread	*thread;
	
	Log("--- Thread Dump ---");
	Threads_DumpActive();
	
	Log("All Threads:");
	for(thread=gAllThreads;thread;thread=thread->GlobalNext)
	{
		Log(" %p %i (%i) - %s (CPU %i)",
			thread, thread->TID, thread->TGID, thread->ThreadName, thread->CurCPU);
		Log("  State %i (%s)", thread->Status, casTHREAD_STAT[thread->Status]);
		switch(thread->Status)
		{
		case THREAD_STAT_MUTEXSLEEP:
			Log("  Mutex Pointer: %p", thread->WaitPointer);
			break;
		case THREAD_STAT_SEMAPHORESLEEP:
			Log("  Semaphore Pointer: %p", thread->WaitPointer);
			Log("  Semaphore Name: %s:%s", 
				((tSemaphore*)thread->WaitPointer)->ModName,
				((tSemaphore*)thread->WaitPointer)->Name
				);
			break;
		case THREAD_STAT_ZOMBIE:
			Log("  Return Status: %i", thread->RetStatus);
			break;
		default:	break;
		}
		Log("  Priority %i, Quantum %i", thread->Priority, thread->Quantum);
		Log("  KStack 0x%x", thread->KernelStack);
		if( thread->bInstrTrace )
			Log("  Tracing Enabled");
		Proc_DumpThreadCPUState(thread);
	}
}

/**
 * \brief Gets the next thread to run
 * \param CPU	Current CPU
 * \param Last	The thread the CPU was running
 */
tThread *Threads_GetNextToRun(int CPU, tThread *Last)
{
	tThread	*thread;
	
	// If this CPU has the lock, we must let it complete
	if( CPU_HAS_LOCK( &glThreadListLock ) )
		return Last;
	
	// Don't change threads if the current CPU has switches disabled
	if( gaThreads_NoTaskSwitch[CPU] )
		return Last;

	// Lock thread list
	SHORTLOCK( &glThreadListLock );
	
	// Make sure the current (well, old) thread is marked as de-scheduled	
	if(Last)	Last->CurCPU = -1;

	// No active threads, just take a nap
	if(giNumActiveThreads == 0) {
		SHORTREL( &glThreadListLock );
		#if DEBUG_TRACE_TICKETS
		Log("No active threads");
		#endif
		return NULL;
	}

	#if 0	
	#if SCHEDULER_TYPE != SCHED_RR_PRI
	// Special case: 1 thread
	if(giNumActiveThreads == 1) {
		if( gActiveThreads.Head->CurCPU == -1 )
			gActiveThreads.Head->CurCPU = CPU;
		
		SHORTREL( &glThreadListLock );
		
		if( gActiveThreads.Head->CurCPU == CPU )
			return gActiveThreads.Head;
		
		return NULL;	// CPU has nothing to do
	}
	#endif
	#endif	

	// Allow the old thread to be scheduled again
	if( Last ) {
		if( Last->Status == THREAD_STAT_ACTIVE ) {
			tThreadList	*list;
			#if SCHEDULER_TYPE == SCHED_LOTTERY
			giFreeTickets += caiTICKET_COUNTS[ Last->Priority ];
			# if DEBUG_TRACE_TICKETS
			LogF("Log: CPU%i released %p (%i %s) into the pool (%i [+%i] tickets in pool)\n",
				CPU, Last, Last->TID, Last->ThreadName, giFreeTickets,
				caiTICKET_COUNTS[ Last->Priority ]);
			# endif
			#endif
			
			#if SCHEDULER_TYPE == SCHED_RR_PRI
			list = &gaActiveThreads[ Last->Priority ];
			#else
			list = &gActiveThreads;
			#endif
			// Add to end of list
			Threads_int_AddToList( list, Last );
		}
		#if SCHEDULER_TYPE == SCHED_LOTTERY && DEBUG_TRACE_TICKETS
		else
			LogF("Log: CPU%i released %p (%i %s)->Status = %i (Released,not in pool)\n",
				CPU, Last, Last->TID, Last->ThreadName, Last->Status);
		#endif
		Last->CurCPU = -1;
	}
	
	// ---
	// Lottery Scheduler
	// ---
	#if SCHEDULER_TYPE == SCHED_LOTTERY
	{
		 int	ticket, number;
		# if 1
		number = 0;
		for(thread = gActiveThreads.Head; thread; thread = thread->Next)
		{
			if(thread->Status != THREAD_STAT_ACTIVE)
				Panic("Bookkeeping fail - %p %i(%s) is on the active queue with a status of %i",
					thread, thread->TID, thread->ThreadName, thread->Status);
			if(thread->Next == thread) {
				Panic("Bookkeeping fail - %p %i(%s) loops back on itself",
					thread, thread->TID, thread->ThreadName, thread->Status);
			}
			number += caiTICKET_COUNTS[ thread->Priority ];
		}
		if(number != giFreeTickets) {
			Panic("Bookkeeping fail (giFreeTickets(%i) != number(%i)) - CPU%i",
				giFreeTickets, number, CPU);
		}
		# endif
		
		// No free tickets (all tasks delegated to cores)
		if( giFreeTickets == 0 ) {
			SHORTREL(&glThreadListLock);
			return NULL;
		}
		
		// Get the ticket number
		ticket = number = rand() % giFreeTickets;
		
		// Find the next thread
		for(thread = gActiveThreads.Head; thread; prev = thread, thread = thread->Next )
		{
			if( caiTICKET_COUNTS[ thread->Priority ] > number)	break;
			number -= caiTICKET_COUNTS[ thread->Priority ];
		}
		
		// If we didn't find a thread, something went wrong
		if(thread == NULL)
		{
			number = 0;
			for(thread=gActiveThreads;thread;thread=thread->Next) {
				if(thread->CurCPU >= 0)	continue;
				number += caiTICKET_COUNTS[ thread->Priority ];
			}
			Panic("Bookeeping Failed - giFreeTickets(%i) > true count (%i)",
				giFreeTickets, number);
		}

		// Remove
		if(prev)
			prev->Next = thread->Next;
		else
			gActiveThreads.Head = thread->Next;
		if(!thread->Next)
			gActiveThreads.Tail = prev;		

		giFreeTickets -= caiTICKET_COUNTS[ thread->Priority ];
		# if DEBUG_TRACE_TICKETS
		LogF("Log: CPU%i allocated %p (%i %s), (%i [-%i] tickets in pool), \n",
			CPU, thread, thread->TID, thread->ThreadName,
			giFreeTickets, caiTICKET_COUNTS[ thread->Priority ]);
		# endif
	}
	
	// ---
	// Priority based round robin scheduler
	// ---
	#elif SCHEDULER_TYPE == SCHED_RR_PRI
	{
		 int	i;
		thread = NULL;
		for( i = 0; i < MIN_PRIORITY + 1; i ++ )
		{
			if( !gaActiveThreads[i].Head )
				continue ;
	
			thread = gaActiveThreads[i].Head;
			
			// Remove from head
			gaActiveThreads[i].Head = thread->Next;
			if(!thread->Next)
				gaActiveThreads[i].Tail = NULL;
			thread->Next = NULL;
			break;
		}
		
		// Anything to do?
		if( !thread ) {
			SHORTREL(&glThreadListLock);
			return NULL;
		}
		if( thread->Status != THREAD_STAT_ACTIVE ) {
			LogF("Oops, Thread %i (%s) is not active\n", thread->TID, thread->ThreadName);
		}
	}
	#elif SCHEDULER_TYPE == SCHED_RR_SIM
	{
		// Get the next thread off the list
		thread = gActiveThreads.Head;	
		gActiveThreads.Head = thread->Next;
		if(!thread->Next)
			gaActiveThreads.Tail = NULL;
		thread->Next = NULL;
		
		// Anything to do?
		if( !thread ) {
			SHORTREL(&glThreadListLock);
			return NULL;
		}
	}
	#else
	# error "Unimplemented scheduling algorithm"
	#endif
	
	// Make the new thread non-schedulable
	thread->CurCPU = CPU;
	thread->Remaining = thread->Quantum;
	
	SHORTREL( &glThreadListLock );
	
	return thread;
}

// === EXPORTS ===
EXPORT(Threads_GetUID);
EXPORT(Threads_GetGID);
