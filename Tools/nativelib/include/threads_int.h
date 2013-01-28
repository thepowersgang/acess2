/*
 * Acess2 libnative (Kernel Simulation Library)
 * - By John Hodge (thePowersGang)
 *
 * threads_int.h
 * - Threads handling definitions
 */
#ifndef _THREADS_INT_H_
#define _THREADS_INT_H_

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

	tThreadIntMutex	*Protector;

	uint32_t	PendingEvents;
	uint32_t	WaitingEvents;
	tThreadIntSem	*WaitSemaphore;	// pthreads

	char	*Name;

	// Init Only
	void	(*SpawnFcn)(void*);
	void	*SpawnData;
};

extern struct sThread __thread	*lpThreads_This;

extern int	Threads_int_CreateThread(struct sThread *Thread);
extern int	Threads_int_ThreadingEnabled(void);

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

