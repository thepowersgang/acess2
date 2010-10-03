/*
 */
#ifndef _THREADS_H_
#define _THREADS_H_

#include <arch.h>
#include <signal.h>
#include <proc.h>

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
 * \brief Core threading structure
 * 
 */
typedef struct sThread
{
	// --- threads.c's
	/**
	 * \brief Next thread in current list
	 * \note Required to be first for linked list hacks to work
	 */
	struct sThread	*Next;
	struct sThread	*GlobalNext;	//!< Next thread in global list
	struct sThread	*GlobalPrev;	//!< Previous thread in global list
	tShortSpinlock	IsLocked;	//!< Thread's spinlock
	volatile int	Status;		//!< Thread Status
	 int	RetStatus;	//!< Return Status
	
	Uint	TID;	//!< Thread ID
	Uint	TGID;	//!< Thread Group (Process)
	struct sThread	*Parent;	//!< Parent Thread
	Uint	UID, GID;	//!< User and Group
	char	*ThreadName;	//!< Name of thread
	
	// --- arch/proc.c's responsibility
	//! Kernel Stack Base
	tVAddr	KernelStack;
	
	//! Memory Manager State
	tMemoryState	MemState;
	
	//! State on task switch
	tTaskState	SavedState;
	
	// --- threads.c's
	 int	CurFaultNum;	//!< Current fault number, 0: none
	tVAddr	FaultHandler;	//!< Fault Handler
	
	tMsg * volatile	Messages;	//!< Message Queue
	tMsg	*LastMessage;	//!< Last Message (speeds up insertion)
	
	 int	Quantum, Remaining;	//!< Quantum Size and remaining timesteps
	 int	Priority;	//!< Priority - 0: Realtime, higher means less time
	
	Uint	Config[NUM_CFG_ENTRIES];	//!< Per-process configuration
	
	volatile int	CurCPU;
} tThread;


enum {
	THREAD_STAT_NULL,	// Invalid process
	THREAD_STAT_ACTIVE,	// Running and schedulable process
	THREAD_STAT_SLEEPING,	// Message Sleep
	THREAD_STAT_OFFSLEEP,	// Mutex Sleep (or waiting on a thread)
	THREAD_STAT_WAITING,	// ???
	THREAD_STAT_PREINIT,	// Being created
	THREAD_STAT_ZOMBIE,	// Died, just not removed
	THREAD_STAT_DEAD	// Why do we care about these???
};

enum eFaultNumbers
{
	FAULT_MISC,
	FAULT_PAGE,
	FAULT_ACCESS,
	FAULT_DIV0,
	FAULT_OPCODE,
	FAULT_FLOAT
};

#define GETMSG_IGNORE	((void*)-1)

// === GLOBALS ===
extern BOOL	gaThreads_NoTaskSwitch[MAX_CPUS];

// === FUNCTIONS ===
extern tThread	*Proc_GetCurThread(void);
extern tThread	*Threads_GetThread(Uint TID);
extern void	Threads_SetPriority(tThread *Thread, int Pri);
extern int	Threads_Wake(tThread *Thread);
extern void	Threads_AddActive(tThread *Thread);
extern tThread	*Threads_GetNextToRun(int CPU, tThread *Last);

#endif
