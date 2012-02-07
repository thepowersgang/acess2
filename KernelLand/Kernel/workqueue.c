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

// === CODE ===
void Workqueue_Init(tWorkqueue *Queue, const char *Name, size_t NextOfset)
{
	Queue->Name = Name;
	Queue->NextOffset = NextOfset;
}

void *Workqueue_GetWork(tWorkqueue *Queue)
{
	tThread	*us;

	for( ;; )
	{
		// Check for work
		SHORTLOCK(&Queue->Protector);
		if(Queue->Head)
		{
			void *ret = Queue->Head;
			Queue->Head = *( (void**)ret + Queue->NextOffset/sizeof(void*) );
			if(Queue->Tail == ret)
				Queue->Tail = NULL;
			SHORTREL(&Queue->Protector);	
			return ret;
		}
		
		// Go to sleep
		SHORTLOCK(&glThreadListLock);
		us = Threads_RemActive();
		us->WaitPointer = Queue;
		us->Status = THREAD_STAT_QUEUESLEEP;
		Queue->Sleeper = us;
		SHORTREL(&Queue->Protector);	
		SHORTREL(&glThreadListLock);
		
		// Yield and sleep
		Threads_Yield();
		if(us->Status == THREAD_STAT_QUEUESLEEP) {
			// Why are we awake?!
		}

		us->WaitPointer = NULL;
	}
}

void Workqueue_AddWork(tWorkqueue *Queue, void *Ptr)
{
	SHORTLOCK(&Queue->Protector);

	if( Queue->Tail )
		*( (void**)Queue->Tail + Queue->NextOffset/sizeof(void*) ) = Ptr;
	else
		Queue->Head = Ptr;
	Queue->Tail = Ptr;
	
	if( Queue->Sleeper )
	{	
		if( Queue->Sleeper->Status != THREAD_STAT_ACTIVE )
			Threads_AddActive(Queue->Sleeper);
		Queue->Sleeper = NULL;
	}
	SHORTREL(&Queue->Protector);
}
