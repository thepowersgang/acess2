#ifndef _THREADS_INT_H_
#define _THREADS_INT_H_

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

typedef struct sProcess
{
	 int	nThreads;
	 int	NativePID;
	char	*CWD;
	char	*Chroot;
	 int	MaxFD;
} tProcess;

struct sThread
{
	struct sThread	*GlobalNext;
	struct sThread	*Next;

	 int	KernelTID;

	tTID	TID, PID;
	tUID	UID, GID;

	struct sThread	*Parent;

	char	*ThreadName;

	 int	Status;	// 0: Dead, 1: Active, 2: Paused, 3: Asleep
	 int	ExitStatus;
	 int	_errno;

	// Threads waiting for this thread to exit.
	// Quit logic:
	// - Wait for `WaitingThreads` to be non-null (maybe?)
	// - Wake first in the queue, wait for it to be removed
	// - Repeat
	// - Free thread and quit kernel thread
	struct sThread	*WaitingThreads;
	struct sThread	*WaitingThreadsEnd;

	tProcess	*Process;	

	Uint32	Events, WaitMask;
	void	*EventSem;	// Should be SDL_sem, but I don't want SDL in this header

	// Message queue
	tMsg * volatile	Messages;	//!< Message Queue
	tMsg	*LastMessage;	//!< Last Message (speeds up insertion)
};

enum {
	THREAD_STAT_NULL,	// Invalid process
	THREAD_STAT_ACTIVE,	// Running and schedulable process
	THREAD_STAT_SLEEPING,	// Message Sleep
	THREAD_STAT_MUTEXSLEEP,	// Mutex Sleep
	THREAD_STAT_SEMAPHORESLEEP,	// Semaphore Sleep
	THREAD_STAT_QUEUESLEEP,	// Queue
	THREAD_STAT_EVENTSLEEP,	// Event sleep
	THREAD_STAT_WAITING,	// ??? (Waiting for a thread)
	THREAD_STAT_PREINIT,	// Being created
	THREAD_STAT_ZOMBIE,	// Died/Killed, but parent not informed
	THREAD_STAT_DEAD,	// Awaiting burial (free)
	THREAD_STAT_BURIED	// If it's still on the list here, something's wrong
};
extern tThread	*Threads_GetThread(Uint TID);

#endif

