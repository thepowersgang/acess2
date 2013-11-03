/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * workqueue.c
 * - Worker FIFO Queue (Single Consumer, Interrupt Producer)
 */
#include <acess.h>
#include <workqueue.h>
#include <threads_int.h>

#define QUEUENEXT(ptr)	(*( (void**)(ptr) + Queue->NextOffset/sizeof(void*) ))

// === CODE ===
void Workqueue_Init(tWorkqueue *Queue, const char *Name, size_t NextOfset)
{
	Queue->Name = Name;
	Queue->NextOffset = NextOfset;
}

void *Workqueue_GetWork(tWorkqueue *Queue)
{
	for( ;; )
	{
		// Check for work
		SHORTLOCK(&Queue->Protector);
		if(Queue->Head)
		{
			void *ret = Queue->Head;
			Queue->Head = QUEUENEXT( ret );
			if(Queue->Tail == ret)
				Queue->Tail = NULL;
			SHORTREL(&Queue->Protector);	
			return ret;
		}
		
		Threads_int_Sleep(THREAD_STAT_QUEUESLEEP,
			Queue, 0,
			&Queue->Sleeper, NULL, &Queue->Protector);
	}
}

void Workqueue_AddWork(tWorkqueue *Queue, void *Ptr)
{
	SHORTLOCK(&Queue->Protector);

	if( Queue->Tail )
		QUEUENEXT(Queue->Tail) = Ptr;
	else
		Queue->Head = Ptr;
	Queue->Tail = Ptr;
	QUEUENEXT(Ptr) = NULL;

	if( Queue->Sleeper )
	{	
		if( Queue->Sleeper->Status != THREAD_STAT_ACTIVE )
			Threads_AddActive(Queue->Sleeper);
		Queue->Sleeper = NULL;
	}
	SHORTREL(&Queue->Protector);
}
