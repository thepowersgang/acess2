/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 * threads.c
 * - Common Thread Control
 */
#include <acess.h>
#include <threads.h>
#include <threads_int.h>
#include <errno.h>
#include <hal_proc.h>
#include <semaphore.h>
#include <rwlock.h>
#include <vfs_threads.h>	// VFS Handle maintainence
#include <events.h>

// Configuration
#define DEBUG_TRACE_ACTIVEQUEUE	0	// Trace adds/removals from the active queue
#define DEBUG_TRACE_TICKETS	0	// Trace ticket counts
#define DEBUG_TRACE_STATE	0	// Trace state changes (sleep/wake)
#define DEBUG_TRACE_SCHEDULE	0	// Trace scheduling

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

#define PRIthread_fmt	"%p(%i %s)"
#define PRIthread_args(t)	(t), ((t)?(t)->TID:-1), ((t)?(t)->ThreadName:"-")

// === IMPORTS ===
extern void	User_Signal_Kill(int SigNum);

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
char	*Threads_GetName(tTID ID);
#if 0
void	Threads_SetPriority(tThread *Thread, int Pri);
tThread	*Threads_CloneTCB(Uint Flags);
 int	Threads_WaitTID(int TID, int *status);
tThread	*Threads_GetThread(Uint TID);
#endif
tThread	*Threads_int_DelFromQueue(tThreadList *List, tThread *Thread);
void	Threads_int_AddToList(tThreadList *List, tThread *Thread);
#if 0
void	Threads_Exit(int TID, int Status);
void	Threads_Kill(tThread *Thread, int Status);
void	Threads_Yield(void);
 int	Threads_int_Sleep(enum eThreadStatus Status, void *Ptr, int Num, tThread **ListHead, tThread **ListTail, tShortSpinlock *Lock);
void	Threads_Sleep(void);
 int	Threads_Wake(tThread *Thread);
void	Threads_AddActive(tThread *Thread);
tThread	*Threads_RemActive(void);
#endif
void	Threads_ToggleTrace(int TID);
void	Threads_Fault(int Num);
void	Threads_SegFault(tVAddr Addr);
void	Threads_PostSignalTo(tThread *Thread, int SignalNum);
#if 0
void	Threads_PostSignal(int SignalNum);
void	Threads_SignalGroup(tPGID PGID, int SignalNum);
#endif
 int	Threads_GetPendingSignal(void);
void	Threads_SetSignalHandler(int SignalNum, void *Handler);
void	*Threads_GetSignalHandler(int SignalNum);
#if 0
 int	Threads_GetPID(void);
 int	Threads_GetTID(void);
tUID	Threads_GetUID(void);
tGID	Threads_GetGID(void);
 int	Threads_SetUID(Uint *Errno, tUID ID);
 int	Threads_SetGID(Uint *Errno, tUID ID);
#endif
void	Threads_int_DumpThread(tThread *thread);
void	Threads_Dump(void);
void	Threads_DumpActive(void);
tThread	*Threads_int_GetRunnable(void);

// === GLOBALS ===
// -- Core Thread --
tProcess	gProcessZero = {
	};
// Only used for the core kernel
tThread	gThreadZero = {
	.Status 	= THREAD_STAT_ACTIVE,	// Status
	.ThreadName	= (char*)"ThreadZero",	// Name
	.Quantum	= DEFAULT_QUANTUM,	// Default Quantum
	.Remaining	= DEFAULT_QUANTUM,	// Current Quantum
	.Priority	= DEFAULT_PRIORITY	// Number of tickets
	};
// -- Processes --
tProcess	*gAllProcesses = &gProcessZero;
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
	gAllThreads = &gThreadZero;
	giNumActiveThreads = 1;
	gThreadZero.Process = &gProcessZero;
		
	Proc_Start();
}

void Threads_Delete(tThread *Thread)
{
	// Set to dead
	Thread->Status = THREAD_STAT_BURIED;

	// Clear out process state
	Proc_ClearThread(Thread);			

	Thread->Process->nThreads --;

	if( Thread->Process->FirstThread == Thread )
	{
		Thread->Process->FirstThread = Thread->ProcessNext;
	}
	else
	{
		tThread	*prev = Thread->Process->FirstThread;
		while(prev && prev->ProcessNext != Thread)
			prev = prev->ProcessNext;
		if( !prev )
			Log_Error("Threads", "Thread %p(%i %s) is not on the process's list",
				Thread, Thread->TID, Thread->ThreadName
				);
		else
			prev->ProcessNext = Thread->ProcessNext;
	}

	// If the final thread is being terminated, clean up the process
	if( Thread->Process->nThreads == 0 )
	{
		tProcess	*proc = Thread->Process;

		// Remove from global process list
		// TODO: RWLock
		if(proc->Prev)
			proc->Prev->Next = proc->Next;
		else
			gAllProcesses = proc->Next;
		if(proc->Next)
			proc->Next->Prev = proc->Prev;

		// VFS Cleanup
		VFS_CloseAllUserHandles();
		// Architecture cleanup
		Proc_ClearProcess( proc );
		// VFS Configuration strings
		if( proc->CurrentWorkingDir)
			free( proc->CurrentWorkingDir );
		if( proc->RootDir )
			free( proc->RootDir );
		// Process descriptor
		free( proc );
	}
	
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

	Log_Debug("Threads", "Thread renamed to '%s'", NewName);	

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
	if(Flags & CLONE_VM) {
		tProcess	*newproc, *oldproc;
		oldproc = cur->Process;
		new->Process = malloc( sizeof(struct sProcess) );
		newproc = new->Process;
		newproc->PID = new->TID;
		if( Flags & CLONE_PGID )
			newproc->PGID = oldproc->PGID;
		else
			newproc->PGID = newproc->PID;
		newproc->UID = oldproc->UID;
		newproc->GID = oldproc->GID;
		newproc->MaxFD = oldproc->MaxFD;
		newproc->CurrentWorkingDir = oldproc->CurrentWorkingDir ? strdup( oldproc->CurrentWorkingDir ) : NULL;
		newproc->RootDir = oldproc->RootDir ? strdup( oldproc->RootDir ) : NULL;
		newproc->nThreads = 1;
		// Reference all handles in the VFS
		VFS_ReferenceUserHandles();
		
		// Add to global list
		newproc->Prev = NULL;
		// TODO: RWLock
		newproc->Next = gAllProcesses;
		gAllProcesses = newproc;

		newproc->FirstThread = new;
		new->ProcessNext = NULL;
	}
	else {
		new->Process->nThreads ++;
		new->Process = cur->Process;
		// TODO: Locking
		new->ProcessNext = new->Process->FirstThread;
		new->Process->FirstThread = new;
	}
	
	// Messages are not inherited
	new->Messages = NULL;
	new->LastMessage = NULL;
	
	// Set State
	new->Remaining = new->Quantum = cur->Quantum;
	new->Priority = cur->Priority;
	new->_errno = 0;
	
	// Set Signal Handlers
	new->CurFaultNum = 0;
	new->FaultHandler = cur->FaultHandler;
	
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
	
	// Allocate and duplicate
	new = malloc(sizeof(tThread));
	if(new == NULL) {
		return NULL;
	}
	memcpy(new, &gThreadZero, sizeof(tThread));

	new->Process->nThreads ++;
	
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
 * \brief Wait for a task to change state
 * \param TID	Thread ID to wait on (-1: Any child thread, 0: Any Child/Sibling, <-1: -PID)
 * \param Status	Thread return status
 * \return TID of child that changed state
 */
tTID Threads_WaitTID(int TID, int *Status)
{	
	// Any Child
	if(TID == -1)
	{
		Uint32 ev = Threads_WaitEvents(THREAD_EVENT_DEADCHILD);
		tTID	ret = -1;
		if( ev & THREAD_EVENT_DEADCHILD )
		{
			// A child died, get the TID
			tThread	*us = Proc_GetCurThread();
			ASSERT(us->LastDeadChild);
			ret = us->LastDeadChild->TID;
			// - Mark as dead (as opposed to undead)
			ASSERT(us->LastDeadChild->Status == THREAD_STAT_ZOMBIE);
			us->LastDeadChild->Status = THREAD_STAT_DEAD;
			// - Set return status
			if(Status)
				*Status = us->LastDeadChild->RetStatus;
			us->LastDeadChild = NULL;
			Mutex_Release(&us->DeadChildLock);
		}
		else
		{
			Log_Error("Threads", "TODO: Threads_WaitTID(TID=-1) - Any Child");
		}
		return ret;
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
	if(TID > 0)
	{
		// TODO: Register on thread to be poked when it dies
		tTID	ret;
		// NOTE: Race condition - Other child dies, desired child dies, first death is 'lost'
		while( (ret = Threads_WaitTID(-1, Status)) != TID )
		{
			if( ret == -1 )
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
	for( thread = gAllThreads; thread; thread = thread->GlobalNext )
	{
		if(thread->TID == TID)
			return thread;
	}

	Log_Notice("Threads", "Unable to find TID %i on main list\n", TID);
	
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
	// TODO: It's possible that we could be timer-preempted here, should disable that... somehow
	Mutex_Acquire( &Thread->Parent->DeadChildLock );	// released by parent
	Thread->Parent->LastDeadChild = Thread;
	Threads_PostEvent( Thread->Parent, THREAD_EVENT_DEADCHILD );
	
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
 * \breif Wait for the thread status to not be a specified value
 */
void Threads_int_WaitForStatusEnd(enum eThreadStatus Status)
{
	tThread	*us = Proc_GetCurThread();
	ASSERT(Status != THREAD_STAT_ACTIVE);
	ASSERT(Status != THREAD_STAT_DEAD);
	while( us->Status == Status )
	{
		Proc_Reschedule();
		if( us->Status == Status )
			Debug("Thread %p(%i %s) rescheduled while in %s state for %p",
				us, us->TID, us->ThreadName,
				casTHREAD_STAT[Status],
				__builtin_return_address(0));
	}
}

int Threads_int_Sleep(enum eThreadStatus Status, void *Ptr, int Num, tThread **ListHead, tThread **ListTail, tShortSpinlock *Lock)
{
	SHORTLOCK( &glThreadListLock );
	tThread	*us = Threads_RemActive();
	us->Next = NULL;
	// - Mark as sleeping
	us->Status = Status;
	us->WaitPointer = Ptr;
	us->RetStatus = Num;	// Use RetStatus as a temp variable
		
	// - Add to waiting
	if( ListTail ) {
		if(*ListTail) {
			(*ListTail)->Next = us;
		}
		else {
			*ListHead = us;
		}
		*ListTail = us;
	}
	else {
		*ListHead = us;
	}
	
	//if( Proc_ThreadSync(us) )
	//	return ;
	SHORTREL( &glThreadListLock );
	if( Lock )
		SHORTREL( Lock );
	Threads_int_WaitForStatusEnd(Status);
	us->WaitPointer = NULL;
	return us->RetStatus;
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
	Threads_int_WaitForStatusEnd(THREAD_STAT_SLEEPING);
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
		// Remove from sleeping queue
		SHORTLOCK( &glThreadListLock );
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
		Log_Warning("Threads", "Threads_Wake - Unknown process status (%i)", Thread->Status);
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
	#if DEBUG_TRACE_ACTIVEQUEUE
	Debug("Threads_AddActive("PRIthread_fmt")", PRIthread_args(Thread));
	#endif
	SHORTLOCK( &glThreadListLock );
	
	if( Thread->Status == THREAD_STAT_ACTIVE )
	{
		tThread	*cur = Proc_GetCurThread();
		Log_KernelPanic("Threads",
			"ret=%p CPU%i "PRIthread_fmt" is adding "PRIthread_fmt" when it is active",
			__builtin_return_address(0),
			GetCPUNum(), PRIthread_args(cur), PRIthread_args(Thread));
		SHORTREL( &glThreadListLock );
		return ;
	}
	
	// Set state
	Thread->Status = THREAD_STAT_ACTIVE;
	// Add to active list
	// - Thread can be the current thread if we're interrupted just before
	//   Proc_Reschedule in a sleep state.
	if( Thread != Proc_GetCurThread() )
	{
		#if SCHEDULER_TYPE == SCHED_RR_PRI
		tThreadList	*list = &gaActiveThreads[Thread->Priority];
		#else
		tThreadList	*list = &gActiveThreads;
		#endif
		#if DEBUG_TRACE_ACTIVEQUEUE
		Debug(" - Head="PRIthread_fmt",Tail="PRIthread_fmt"",
			PRIthread_args(list->Head),
			PRIthread_args(list->Tail)
			);
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
	tThread	*us = Proc_GetCurThread();
	#if DEBUG_TRACE_ACTIVEQUEUE
	Debug("Threads_RemActive(%p(%i %s))", us, us->TID, us->ThreadName);
	#endif
	giNumActiveThreads --;
	return us;
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


void Threads_PostSignalTo(tThread *Thread, int SignalNum)
{
	ASSERT(Thread);
	Log_Debug("Threads", "Signalling %i(%s) with %i", Thread->TID, Thread->ThreadName, SignalNum);
	Thread->PendingSignal = SignalNum;
	Threads_PostEvent(Thread, THREAD_EVENT_SIGNAL);
}
void Threads_PostSignal(int SignalNum)
{
	Threads_PostSignalTo( Proc_GetCurThread(), SignalNum );
}

void Threads_SignalGroup(tPGID PGID, int Signal)
{
	for( tProcess *proc = gAllProcesses; proc; proc = proc->Next )
	{
		if(proc->PGID == PGID)
		{
			Threads_PostSignalTo(proc->FirstThread, Signal);
		}
	}
}

/**
 */
int Threads_GetPendingSignal(void)
{
	tThread *cur = Proc_GetCurThread();
	
	// Atomic AND with 0 fetches and clears in one operation
	int ret = __sync_fetch_and_and( &cur->PendingSignal, 0 );
	if( ret )
	{
		Log_Debug("Threads", "Thread %i(%s) has signal %i pending",
			cur->TID, cur->ThreadName, ret);
	}
	return ret;
}

/*
 * \brief Update the current thread's signal handler
 */
void Threads_SetSignalHandler(int SignalNum, void *Handler)
{
	if( SignalNum <= 0 || SignalNum >= NSIGNALS )
		return ;
	if( !MM_IsUser(Handler) )
		return ;
	Proc_GetCurThread()->Process->SignalHandlers[SignalNum] = Handler;
}

/**
 * \brief Gets the registered (or default, if none set) handler for a signal.
 * \return Handler function pointer, OR NULL if no signal to be ignored
 */
void *Threads_GetSignalHandler(int SignalNum)
{
	// TODO: Core dump
	void *User_Signal_Core = User_Signal_Kill;
	
	if( SignalNum <= 0 || SignalNum >= NSIGNALS )
		return NULL;
	void *ret = Proc_GetCurThread()->Process->SignalHandlers[SignalNum];
	if( !ret || (SignalNum == SIGKILL || SignalNum == SIGSTOP) )
	{
		// Defaults
		switch(SignalNum)
		{
		case SIGHUP:
		case SIGINT:
			ret = User_Signal_Kill;
			break;
		case SIGQUIT:
		case SIGILL:
		case SIGABRT:
		case SIGFPE:
			ret = User_Signal_Core;
			break;
		case SIGKILL:
			ret = User_Signal_Kill;
			break;
		case SIGSEGV:
			ret = User_Signal_Core;
			break;
		case SIGPIPE:
		case SIGALRM:
		case SIGTERM:
			ret = User_Signal_Kill;
			break;
		default:
			ret = NULL;
			break;
		}
	}
	Log_Debug("Threads", "Handler %p for signal %i", ret, SignalNum);
	return ret;
}

// --- Process Structure Access Functions ---
tPGID Threads_GetPGID(void)
{
	return Proc_GetCurThread()->Process->PGID;
}
tPID Threads_GetPID(void)
{
	return Proc_GetCurThread()->Process->PID;
}
tTID Threads_GetTID(void)
{
	return Proc_GetCurThread()->TID;
}
tUID Threads_GetUID(void)
{
	return Proc_GetCurThread()->Process->UID;
}
tGID Threads_GetGID(void)
{
	return Proc_GetCurThread()->Process->GID;
}

int Threads_SetUID(tUID ID)
{
	tThread	*t = Proc_GetCurThread();
	if( t->Process->UID != 0 ) {
		errno = -EACCES;
		return -1;
	}
	Log_Debug("Threads", "PID %i's UID set to %i", t->Process->PID, ID);
	t->Process->UID = ID;
	return 0;
}

int Threads_SetGID(tGID ID)
{
	tThread	*t = Proc_GetCurThread();
	if( t->Process->UID != 0 ) {
		errno = -EACCES;
		return -1;
	}
	Log_Debug("Threads", "PID %i's GID set to %i", t->Process->PID, ID);
	t->Process->GID = ID;
	return 0;
}

// --- Per-thread storage ---
int *Threads_GetErrno(void)
{
	return &Proc_GetCurThread()->_errno;
}

// --- Configuration ---
int *Threads_GetMaxFD(void)
{
	return &Proc_GetCurThread()->Process->MaxFD;
}
char **Threads_GetChroot(void)
{
	return &Proc_GetCurThread()->Process->RootDir;
}
char **Threads_GetCWD(void)
{
	return &Proc_GetCurThread()->Process->CurrentWorkingDir;
}
// ---

void Threads_int_DumpThread(tThread *thread)
{
	if( !thread ) {
		Log(" %p NULL", thread);
		return ;
	}
	if( !CheckMem(thread, sizeof(tThread)) ) {
		Log(" %p INVAL", thread);
		return ;
	}
	tPID	pid = (thread->Process ? thread->Process->PID : -1);
	const char	*statstr = (thread->Status < sizeof(casTHREAD_STAT)/sizeof(casTHREAD_STAT[0])
		? casTHREAD_STAT[thread->Status] : "");
	Log(" %p %i (%i) - %s (CPU %i) - %i (%s)",
		thread, thread->TID, pid, thread->ThreadName, thread->CurCPU,
		thread->Status, statstr
		);
	switch(thread->Status)
	{
	case THREAD_STAT_MUTEXSLEEP:
		Log("  Mutex Pointer: %p", thread->WaitPointer);
		break;
	case THREAD_STAT_RWLOCKSLEEP:
		Log("  Lock Pointer: %p", thread->WaitPointer);
		Log("  Lock Name: %s",
			((tRWLock*)thread->WaitPointer)->Name);
		break;
	case THREAD_STAT_SEMAPHORESLEEP:
		Log("  Semaphore Pointer: %p", thread->WaitPointer);
		Log("  Semaphore Name: %s:%s", 
			((tSemaphore*)thread->WaitPointer)->ModName,
			((tSemaphore*)thread->WaitPointer)->Name
			);
		break;
	case THREAD_STAT_EVENTSLEEP:
		Log("  Event Mask: %x", thread->RetStatus);
		break;
	case THREAD_STAT_ZOMBIE:
		Log("  Return Status: %i", thread->RetStatus);
		break;
	default:	break;
	}
	Log("  Priority %i, Quantum %i", thread->Priority, thread->Quantum);
	Log("  KStack %p", thread->KernelStack);
	if( thread->bInstrTrace )
		Log("  Tracing Enabled");
	Proc_DumpThreadCPUState(thread);
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
			Threads_int_DumpThread(thread);
			if(thread->Status != THREAD_STAT_ACTIVE)
				Log("  ERROR State (%i) != THREAD_STAT_ACTIVE (%i)",
					thread->Status, THREAD_STAT_ACTIVE);
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
	Log("--- Thread Dump ---");
	Threads_DumpActive();
	
	Log("All Threads:");
	for(tThread *thread = gAllThreads; thread; thread = thread->GlobalNext)
	{
		Threads_int_DumpThread(thread);
	}
}

tThread *Threads_int_GetRunnable(void)
{
	#if SCHEDULER_TYPE == SCHED_LOTTERY
	// -----------------------------------
	// Lottery Scheduler
	// -----------------------------------
	#if DEBUG_VALIDATE_TICKET_COUNTS
	{
		int total_tickets = 0;
		for(const tThread *thread = gActiveThreads.Head; thread; thread = thread->Next)
		{
			if(thread->Status != THREAD_STAT_ACTIVE)
				Panic("Bookkeeping fail - %p %i(%s) is on the active queue with a status of %i",
					thread, thread->TID, thread->ThreadName, thread->Status);
			if(thread->Next == thread) {
				Panic("Bookkeeping fail - %p %i(%s) loops back on itself",
					thread, thread->TID, thread->ThreadName, thread->Status);
			}
			total_tickets += caiTICKET_COUNTS[ thread->Priority ];
		}
		if(total_tickets != giFreeTickets)
		{
			Panic("Bookkeeping fail (giFreeTickets(%i) != number(%i)) - CPU%i",
				giFreeTickets, total_tickets, CPU);
		}
	}
	# endif
	
	// No free tickets (all tasks delegated to cores)
	if( giFreeTickets == 0 )
	{
		return NULL;
	}
	
	// Get the ticket number
	int ticket = rand() % giFreeTickets;
	int number = ticket;
	
	// Find the next thread
	tThread	**pnp = &gActiveThreads.Head;
	tThread	*thread;
	for(thread = gActiveThreads.Head; thread; pnp = &thread->Next, thread = thread->Next )
	{
		if( caiTICKET_COUNTS[ thread->Priority ] > number)
			break;
		number -= caiTICKET_COUNTS[ thread->Priority ];
	}
	
	// If we didn't find a thread, something went wrong
	if(thread == NULL)
	{
		int total_tickets = 0;
		for(thread=gActiveThreads;thread;thread=thread->Next) {
			if(thread->CurCPU >= 0)	continue;
			total_tickets += caiTICKET_COUNTS[ thread->Priority ];
		}
		Panic("Bookeeping Failed - giFreeTickets(%i) > true count (%i)",
			giFreeTickets, total_tickets);
	}

	// Remove
	*pnp = thread->Next;
	if( !thread->Next )
		gActiveThreads.Tail = prev;		

	giFreeTickets -= caiTICKET_COUNTS[ thread->Priority ];
	# if DEBUG_TRACE_TICKETS
	LogF("Log: CPU%i allocated %p (%i %s), (%i [-%i] tickets in pool), \n",
		CPU, thread, thread->TID, thread->ThreadName,
		giFreeTickets, caiTICKET_COUNTS[ thread->Priority ]);
	# endif
	return thread;
	
	#elif SCHEDULER_TYPE == SCHED_RR_PRI
	
	// -----------------------------------
	// Priority based round robin scheduler
	// -----------------------------------
	for( int i = 0; i < MIN_PRIORITY + 1; i ++ )
	{
		if( gaActiveThreads[i].Head == NULL )
			continue ;
	
		tThread *thread = gaActiveThreads[i].Head;

		if( thread->Status != THREAD_STAT_ACTIVE ) {
			for( const tThread *t = gaActiveThreads[i].Head; t; t = t->Next )
				LogF("- %p(%i %s)\n", t, t->TID, t->ThreadName);
			Panic("Thread %p(%i %s) from pqueue %i is active!",
				thread, thread->TID, thread->ThreadName, i);
		}

		// Remove from head
		gaActiveThreads[i].Head = thread->Next;
		if(!thread->Next)
			gaActiveThreads[i].Tail = NULL;
		thread->Next = NULL;
		return thread;
	}
	return NULL;
	
	#elif SCHEDULER_TYPE == SCHED_RR_SIM
	
	// -----------------------------------
	// Single-list round-robin
	// -----------------------------------
	tThread *thread = gActiveThreads.Head;
	if( thread )
	{
		gActiveThreads.Head = thread->Next;
		if(!thread->Next)
			gaActiveThreads.Tail = NULL;
		thread->Next = NULL;
	}
	return thread;
	#else
	# error "Unimplemented scheduling algorithm"
	return NULL;
	#endif
}

/**
 * \brief Gets the next thread to run
 * \param CPU	Current CPU
 * \param Last	The thread the CPU was running
 */
tThread *Threads_GetNextToRun(int CPU, tThread *Last)
{
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

	// Allow the old thread to be scheduled again
	if( Last )
	{
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
			#if DEBUG_TRACE_ACTIVEQUEUE > 1
			Debug("Threads_GetNextToRun: Append thread %p(%i %s)",
				Last, Last->TID, Last->ThreadName);
			#endif
		}
		#if SCHEDULER_TYPE == SCHED_LOTTERY && DEBUG_TRACE_TICKETS
		else
			LogF("Log: CPU%i released %p (%i %s)->Status = %i (Released,not in pool)\n",
				CPU, Last, Last->TID, Last->ThreadName, Last->Status);
		#endif
		Last->CurCPU = -1;
	}

	// Call actual scheduler	
	tThread	*thread = Threads_int_GetRunnable();
		
	// Anything to do?
	if( thread )
	{
		if( thread->Status != THREAD_STAT_ACTIVE )
		{
			LogF("Thread %p(%i %s) scheduled while not active\n",
				thread, thread->TID, thread->ThreadName);
		}

		// Make the new thread non-schedulable
		thread->CurCPU = CPU;
		thread->Remaining = thread->Quantum;
		
		#if DEBUG_TRACE_SCHEDULE
		Debug("Scheduled "PRIthread_fmt", next = %p",
			PRIthread_args(thread),
			thread->Next);
		#endif
	}
	else
	{
		// No thread possible, warning condition (idle thread should be runnable)
		Warning("No runnable thread for CPU%i", CPU);
	}
	
	SHORTREL( &glThreadListLock );
	
	return thread;
}

// === EXPORTS ===
EXPORT(Threads_GetUID);
EXPORT(Threads_GetGID);
