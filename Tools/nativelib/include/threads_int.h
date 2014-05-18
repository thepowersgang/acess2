/*
 * Acess2 libnative (Kernel Simulation Library)
 * - By John Hodge (thePowersGang)
 *
 * threads_int.h
 * - Threads handling definitions
 */
#ifndef _THREADS_INT_H_
#define _THREADS_INT_H_

#include <shortlock.h>

#ifndef _THREADS_H_
typedef struct sThread	tThread;
#endif

#define THREAD_EVENT_RWLOCK	(1 << 8)

typedef struct sThreadIntMutex	tThreadIntMutex;	// actually pthreads
typedef struct sThreadIntSem	tThreadIntSem;

struct sProcess
{
	struct sProcess	*Next;
	struct sThread	*Threads;

	 int	PID;
	 int	UID, GID;
	
	char	*CWD;
	char	*Chroot;

	 int	MaxFDs;
	void	*Handles;
};

struct sThread
{
	struct sThread	*Next;
	struct sThread	*ListNext;

	struct sProcess	*Process;
	struct sThread	*ProcNext;

	void	*ThreadHandle;
	 int	TID;

	 int	Status;

	tShortSpinlock	IsLocked;

	uint32_t	EventState;
	uint32_t	WaitingEvents;
	tThreadIntSem	*WaitSemaphore;	// pthreads

	char	*ThreadName;

	 int	bInstrTrace;	// Used for semaphore tracing

	 int	RetStatus;
	void	*WaitPointer;

	// Init Only
	void	(*SpawnFcn)(void*);
	void	*SpawnData;
};

enum eThreadStatus {
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
	"THREAD_STAT_RWLOCKSLEEP",
	"THREAD_STAT_SEMAPHORESLEEP",
	"THREAD_STAT_QUEUESLEEP",
	"THREAD_STAT_EVENTSLEEP",
	"THREAD_STAT_WAITING",
	"THREAD_STAT_PREINIT",
	"THREAD_STAT_ZOMBIE",
	"THREAD_STAT_DEAD",
	"THREAD_STAT_BURIED"
};

extern struct sThread __thread	*lpThreads_This;
extern tShortSpinlock	glThreadListLock;

extern int	Threads_int_CreateThread(struct sThread *Thread);
extern int	Threads_int_ThreadingEnabled(void);


extern struct sThread	*Proc_GetCurThread(void);
extern struct sThread	*Threads_RemActive(void);
extern void	Threads_AddActive(struct sThread *Thread);
extern void	Threads_int_WaitForStatusEnd(enum eThreadStatus Status);
extern int	Threads_int_Sleep(enum eThreadStatus Status, void *Ptr, int Num, struct sThread **ListHead, struct sThread **ListTail, tShortSpinlock *Lock);
extern void	Semaphore_ForceWake(struct sThread *Thread);

extern tThreadIntMutex	*Threads_int_MutexCreate(void);
extern void	Threads_int_MutexDestroy(tThreadIntMutex *Mutex);
extern void	Threads_int_MutexLock(tThreadIntMutex *Mutex);
extern void	Threads_int_MutexRelease(tThreadIntMutex *Mutex);

extern tThreadIntSem	*Threads_int_SemCreate(void);
extern void	Threads_int_SemDestroy(tThreadIntSem *Sem);
extern void	Threads_int_SemSignal(tThreadIntSem *Sem);
extern void	Threads_int_SemWait(tThreadIntSem *Sem);
extern void	Threads_int_SemWaitAll(tThreadIntSem *Sem);

#endif

