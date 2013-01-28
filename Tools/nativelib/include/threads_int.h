/*
 * Acess2 libnative (Kernel Simulation Library)
 * - By John Hodge (thePowersGang)
 *
 * threads_int.h
 * - Threads handling definitions
 */
#ifndef _THREADS_INT_H_
#define _THREADS_INT_H_

typedef struct sThreadIntMutex	tThreadIntMutex;	// actually pthreads
typedef struct sThreadIntSem	tThreadIntSem;

struct sProcess
{
	struct sProcess	*Next;

	 int	PID;
	 int	UID, GID;
	
	char	*CWD;
	char	*Chroot;
	 int	MaxFDs;
};

struct sThread
{
	struct sThread	*Next;
	 int	TID;

	tThreadIntMutex	*Protector;

	uint32_t	PendingEvents;
	uint32_t	WaitingEvents;
	tThreadIntSem	*WaitSemaphore;	// pthreads
	
	// Init Only
	void	(*SpawnFcn)(void*);
	void	*SpawnData;
};

extern int	Threads_int_CreateThread(struct sThread *Thread);

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

