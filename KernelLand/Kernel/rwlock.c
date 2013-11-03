/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * rwlock.c
 * - Reader-Writer Lockes
 */
#define DEBUG	0
#include <acess.h>
#include <threads_int.h>
#include <rwlock.h>

// === PROTOTYPES ===
//
// Acquire as a reader (see rwlock.h for documentation)
//
int RWLock_AcquireRead(tRWLock *Lock)
{
	LOG("Acquire RWLock Read %p", Lock);
	// Get protector
	SHORTLOCK( &Lock->Protector );
	
	// Check if the lock is already held by a writer
	if( Lock->Owner )
	{
		LOG("Waiting");
		Threads_int_Sleep(THREAD_STAT_RWLOCKSLEEP, Lock, 0,
			&Lock->ReaderWaiting, &Lock->ReaderWaitingLast,
			&Lock->Protector);
	}
	// Ooh, no problems then!
	else
	{
		Lock->Level++;
		SHORTREL( & Lock->Protector );
	}
	LOG("Obtained");
	
	return 0;
}

int RWLock_AcquireWrite(tRWLock *Lock)
{
	LOG("Acquire RWLock Write %p", Lock);
	
	SHORTLOCK(&Lock->Protector);
	if( Lock->Owner || Lock->Level != 0 )
	{
		LOG("Waiting");
		Threads_int_Sleep(THREAD_STAT_RWLOCKSLEEP, Lock, 0,
			&Lock->WriterWaiting, &Lock->WriterWaitingLast,
			&Lock->Protector);
	}
	else
	{
		// Nothing else is using the lock, nice :)
		Lock->Owner = Proc_GetCurThread();
		SHORTREL(&Lock->Protector);
	}
	LOG("Obtained");
	return 0;
}

// Release a mutex
void RWLock_Release(tRWLock *Lock)
{
	LOG("Release RWLock %p", Lock);
	SHORTLOCK( &Lock->Protector );

	if( Lock->Owner != Proc_GetCurThread() )
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
