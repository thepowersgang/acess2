/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * rwlock.c
 * - Reader-Writer Lockes
 */
#include <acess.h>
#include <threads_int.h>
#include <rwlock.h>

// === PROTOTYPES ===
//
// Acquire as a reader (see rwlock.h for documentation)
//
int RWLock_AcquireRead(tRWLock *Lock)
{
	tThread	*us;
	// Get protector
	SHORTLOCK( &Lock->Protector );
	
	// Check if the lock is already held by a writer
	if( Lock->Owner )
	{
		SHORTLOCK( &glThreadListLock );
		
		// - Remove from active list
		us = Threads_RemActive();
		us->Next = NULL;
		// - Mark as sleeping
		us->Status = THREAD_STAT_RWLOCKSLEEP;
		us->WaitPointer = Lock;
		
		// - Add to waiting
		if(Lock->ReaderWaiting)
			Lock->ReaderWaitingLast->Next = us;
		else
			Lock->ReaderWaiting = us;
		Lock->ReaderWaitingLast = us;
		
		#if DEBUG_TRACE_STATE
		Log("%p (%i %s) waiting on rwlock %p",
			us, us->TID, us->ThreadName, Lock);
		#endif
		
		SHORTREL( &glThreadListLock );
		SHORTREL( &Lock->Protector );
		while(us->Status == THREAD_STAT_RWLOCKSLEEP)	Threads_Yield();
		// We're only woken when we get the lock
		// TODO: Handle when this isn't the case
		us->WaitPointer = NULL;
	}
	// Ooh, no problems then!
	else
	{
		Lock->Level++;
		SHORTREL( & Lock->Protector );
	}
	
	return 0;
}

int RWLock_AcquireWrite(tRWLock *Lock)
{
	tThread	*us;
	
	SHORTLOCK(&Lock->Protector);
	if( Lock->Owner || Lock->Level != 0 )
	{
		SHORTLOCK(&glThreadListLock);
		
		us = Threads_RemActive();
		us->Next = NULL;
		us->Status = THREAD_STAT_RWLOCKSLEEP;
		us->WaitPointer = Lock;
		
		if( Lock->WriterWaiting )
			Lock->WriterWaitingLast->Next = us;
		else
			Lock->WriterWaiting = us;
		Lock->WriterWaitingLast = us;
		
		SHORTREL( &glThreadListLock );
		SHORTREL( &Lock->Protector );
		
		while(us->Status == THREAD_STAT_RWLOCKSLEEP)	Threads_Yield();
		us->WaitPointer = NULL;
	}
	else
	{
		// Nothing else is using the lock, nice :)
		Lock->Owner = Proc_GetCurThread();
		SHORTREL(&Lock->Protector);
	}
	return 0;
}

// Release a mutex
void RWLock_Release(tRWLock *Lock)
{
	SHORTLOCK( &Lock->Protector );

	if( Lock->Owner == Proc_GetCurThread() )
		Lock->Level --;
	
	// Writers first
	if( Lock->WriterWaiting )
	{
		Lock->Owner = Lock->WriterWaiting;	// Set owner
		Lock->WriterWaiting = Lock->WriterWaiting->Next;	// Next!
		
		// Wake new owner
		if( Lock->Owner->Status != THREAD_STAT_ACTIVE )
			Threads_AddActive(Lock->Owner);
	}
	else
	{
		Lock->Owner = NULL;
		
		while( Lock->ReaderWaiting ) {
			Lock->Level ++;
			Threads_AddActive(Lock->ReaderWaiting);
			Lock->ReaderWaiting = Lock->ReaderWaiting->Next;
		}
	}
	SHORTREL( &Lock->Protector );
}

// === EXPORTS ===
EXPORT(RWLock_AcquireRead);
EXPORT(RWLock_AcquireWrite);
EXPORT(RWLock_Release);
