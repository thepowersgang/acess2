/*
 * Acess2 libnative (Kernel Simulation Library)
 * - By John Hodge (thePowersGang)
 *
 * rwlock.c
 * - Read/Write locks
 */
#include <acess.h>
#include <rwlock.h>
#include <events.h>
#include <threads_int.h>

// === CODE ===
int RWLock_AcquireRead(tRWLock *Lock)
{
	SHORTLOCK(&Lock->Protector);
	while( Lock->Level < 0 || Lock->WriterWaiting )
	{
		// Wait
		tThread	*me = Proc_GetCurThread();
		me->ListNext = NULL;
		if( Lock->ReaderWaiting )
			Lock->ReaderWaitingLast->Next = me;
		else
			Lock->ReaderWaiting = me;
		Lock->ReaderWaitingLast = me;
		SHORTREL(&Lock->Protector);
		Threads_WaitEvents(THREAD_EVENT_RWLOCK);
		SHORTLOCK(&Lock->Protector);
	}
	Lock->Level ++;
	SHORTREL(&Lock->Protector);
	return 0;
}

int RWLock_AcquireWrite(tRWLock *Lock)
{
	SHORTLOCK(&Lock->Protector);

	while( Lock->Level != 0 )
	{
		tThread	*me = Proc_GetCurThread();
		me->ListNext = NULL;
		if( Lock->WriterWaiting )
			Lock->WriterWaitingLast->Next = me;
		else
			Lock->WriterWaiting = me;

		SHORTREL(&Lock->Protector);
		Threads_WaitEvents(THREAD_EVENT_RWLOCK);
		SHORTLOCK(&Lock->Protector);
	}
	Lock->Level = -1;
	SHORTREL(&Lock->Protector);
	return 0;
}

void RWLock_Release(tRWLock *Lock)
{
	SHORTLOCK(&Lock->Protector);
	if( Lock->Level == -1 || Lock->Level == 1 )
	{
		// Last reader or a writer yeilding
		// - Yields to writers first, then readers
		if( Lock->WriterWaiting )
		{
			Threads_PostEvent(Lock->WriterWaiting, THREAD_EVENT_RWLOCK);
			Lock->WriterWaiting = Lock->WriterWaiting->ListNext;
		}
		else
		{
			while(Lock->ReaderWaiting)
			{
				Threads_PostEvent(Lock->ReaderWaiting, THREAD_EVENT_RWLOCK);
				Lock->ReaderWaiting = Lock->ReaderWaiting->ListNext;
			}
		}
		// Nobody to wake, oh well
		Lock->Level = 0;
	}
	else if( Lock->Level == 0 )
	{
		// ... oops?
	}
	else
	{
		Lock->Level --;
	}
	SHORTREL(&Lock->Protector);
}

