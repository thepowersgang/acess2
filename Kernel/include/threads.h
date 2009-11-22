/*
 */
#ifndef _THREADS_H_
#define _THREADS_H_

#include <arch.h>
#include <signal.h>

typedef struct sMessage
{
	struct sMessage	*Next;
	Uint	Source;
	Uint	Length;
	Uint8	Data[];
} tMsg;	// sizeof = 12+

typedef struct sThread
{
	// --- threads.c's
	struct sThread	*Next;	//!< Next thread in list
	tSpinlock	IsLocked;	//!< Thread's spinlock
	 int	Status;		//!< Thread Status
	 int	RetStatus;	//!< Return Status
	
	Uint	TID;	//!< Thread ID
	Uint	TGID;	//!< Thread Group (Process)
	Uint	PTID;	//!< Parent Thread ID
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
	 int	CurSignal;	//!< Signal currently being handled (0 for none)
	tVAddr	SignalHandlers[NSIG];	//!< Signal Handler List
	tTaskState	SignalState;	//!< Saved state for signal handler
	
	tMsg * volatile	Messages;	//!< Message Queue
	tMsg	*LastMessage;	//!< Last Message (speeds up insertion)
	
	 int	Quantum, Remaining;	//!< Quantum Size and remaining timesteps
	 int	NumTickets;	//!< Priority - Chance of gaining CPU
	
	Uint	Config[NUM_CFG_ENTRIES];	//!< Per-process configuration
} tThread;


enum {
	THREAD_STAT_NULL,
	THREAD_STAT_ACTIVE,
	THREAD_STAT_SLEEPING,
	THREAD_STAT_WAITING,
	THREAD_STAT_ZOMBIE,
	THREAD_STAT_DEAD
};

// === FUNCTIONS ===
extern tThread	*Proc_GetCurThread();
extern tThread	*Threads_GetThread(Uint TID);
extern void	Threads_Wake(tThread *Thread);
extern void	Threads_AddActive(tThread *Thread);

#endif
