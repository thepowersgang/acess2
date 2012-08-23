/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * include/threads_int.h
 * - Internal Threading header
 * - Only for use by stuff that needs access to the thread type.
 */
#ifndef _THREADS_INT_H_
#define _THREADS_INT_H_

#include <threads.h>
#include <proc.h>
#include <timers_int.h>

typedef struct sProcess	tProcess;

/**
 * \brief IPC Message
 */
typedef struct sMessage
{
	struct sMessage	*Next;	//!< Next message in thread's inbox
	tTID	Source;	//!< Source thread ID
	Uint	Length;	//!< Length of message data in bytes
	Uint8	Data[];	//!< Message data
} tMsg;

/**
 * \brief Process state
 */
struct sProcess
{
	struct sProcess	*Next;
	tPGID	PGID;
	tPID	PID;

	 int	nThreads;
	struct sThread	*FirstThread;
	
	tUID	UID;	//!< User ID
	tGID	GID;	//!< User and Group
	tMemoryState	MemState;

	 int	MaxFD;
	char	*CurrentWorkingDir;
	char	*RootDir;
};

/**
 * \brief Core threading structure
 * 
 */
struct sThread
{
	// --- threads.c's
	/**
	 * \brief Next thread in current list
	 * \note Required to be first for linked list hacks to work
	 */
	struct sThread	*Next;
	struct sThread	*GlobalNext;	//!< Next thread in global list
	struct sThread	*GlobalPrev;	//!< Previous thread in global list
	struct sThread	*ProcessNext;
	tShortSpinlock	IsLocked;	//!< Thread's spinlock
	volatile int	Status;		//!< Thread Status
	void	*WaitPointer;	//!< What (Mutex/Thread/other) is the thread waiting on
	 int	RetStatus;	//!< Return Status
	
	tTID	TID;	//!< Thread ID
	struct sProcess	*Process;	//!< Thread Group / Process
	struct sThread	*Parent;	//!< Parent Thread
	char	*ThreadName;	//!< Name of thread
	
	// --- arch/proc.c's responsibility
	//! Kernel Stack Base
	tVAddr	KernelStack;
	
	//! State on task switch
	tTaskState	SavedState;
	
	// --- threads.c's
	 int	CurFaultNum;	//!< Current fault number, 0: none
	tVAddr	FaultHandler;	//!< Fault Handler
	
	tMsg * volatile	Messages;	//!< Message Queue
	tMsg	*LastMessage;	//!< Last Message (speeds up insertion)
	
	 int	Quantum, Remaining;	//!< Quantum Size and remaining timesteps
	 int	Priority;	//!< Priority - 0: Realtime, higher means less time
	
	 int	_errno;
	
	volatile int	CurCPU;
	
	bool	bInstrTrace;
	
	// --- event.c
	Uint32	EventState;
	// --- timer.c
	tTimer	ThreadTimer;
};


enum {
	THREAD_STAT_NULL,	// Invalid process
	THREAD_STAT_ACTIVE,	// Running and schedulable process
	THREAD_STAT_SLEEPING,	// Message Sleep
	THREAD_STAT_MUTEXSLEEP,	// Mutex Sleep
	THREAD_STAT_RWLOCKSLEEP,	// Read-Writer lock Sleep
	THREAD_STAT_SEMAPHORESLEEP,	// Semaphore Sleep
	THREAD_STAT_QUEUESLEEP,	// Queue
	THREAD_STAT_EVENTSLEEP,	// Event sleep
	THREAD_STAT_WAITING,	// ??? (Waiting for a thread)
	THREAD_STAT_PREINIT,	// Being created
	THREAD_STAT_ZOMBIE,	// Died/Killed, but parent not informed
	THREAD_STAT_DEAD,	// Awaiting burial (free)
	THREAD_STAT_BURIED	// If it's still on the list here, something's wrong
};
static const char * const casTHREAD_STAT[] = {
	"THREAD_STAT_NULL",
	"THREAD_STAT_ACTIVE",
	"THREAD_STAT_SLEEPING",
	"THREAD_STAT_MUTEXSLEEP",
	"THREAD_STAT_SEMAPHORESLEEP",
	"THREAD_STAT_QUEUESLEEP",
	"THREAD_STAT_EVENTSLEEP",
	"THREAD_STAT_WAITING",
	"THREAD_STAT_PREINIT",
	"THREAD_STAT_ZOMBIE",
	"THREAD_STAT_DEAD",
	"THREAD_STAT_BURIED"
};

// === GLOBALS ===
extern BOOL	gaThreads_NoTaskSwitch[MAX_CPUS];
extern tShortSpinlock	glThreadListLock;

// === FUNCTIONS ===
extern tThread	*Threads_GetThread(Uint TID);
extern void	Threads_SetPriority(tThread *Thread, int Pri);
extern int	Threads_Wake(tThread *Thread);
extern void	Threads_Kill(tThread *Thread, int Status);
extern void	Threads_AddActive(tThread *Thread);
extern tThread	*Threads_RemActive(void);
extern void	Threads_Delete(tThread *Thread);
extern tThread	*Threads_GetNextToRun(int CPU, tThread *Last);

extern tThread	*Threads_CloneTCB(Uint Flags);
extern tThread	*Threads_CloneThreadZero(void);

#endif
