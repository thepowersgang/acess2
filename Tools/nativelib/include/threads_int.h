/*
 * Acess2 libnative (Kernel Simulation Library)
 * - By John Hodge (thePowersGang)
 *
 * threads_int.h
 * - Threads handling definitions
 */
#ifndef _THREADS_INT_H_
#define _THREADS_INT_H_

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
	
	uint32_t	PendingEvents;
	uint32_t	WaitingEvents;
	void	*WaitSemaphore;	// pthreads
	
	// Init Only
	void	(*SpawnFcn)(void*);
	void	*SpawnData;
};

extern int	Threads_int_CreateThread(struct sThread *Thread);

#endif

