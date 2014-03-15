/**
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * workqueue.h
 * - FIFO Queue
 */
#ifndef _WORKQUEUE_H_
#define _WORKQUEUE_H_

#include <acess.h>

typedef struct sWorkqueue	tWorkqueue;

struct sWorkqueue
{
	tShortSpinlock	Protector;
	const char	*Name;
	 int	NextOffset;
	
	void	*Head;
	void	*Tail;
	struct sThread	*Sleeper;
	struct sThread	*SleepTail;
};

extern void	Workqueue_Init(tWorkqueue *Queue, const char *Name, size_t NextOfset);
extern void	*Workqueue_GetWork(tWorkqueue *Queue);
extern void	Workqueue_AddWork(tWorkqueue *Queue, void *Ptr);

#endif

