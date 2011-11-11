/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * mutex.c
 * - Mutexes
 */
#include <acess.h>
#include <threads_int.h>
#include <mutex.h>

// === PROTOTYPES ===
#if 0
 int	Mutex_Acquire(tMutex *Mutex);
void	Mutex_Release(tMutex *Mutex);
 int	Mutex_IsLocked(tMutex *Mutex);
#endif

// === CODE ===
//
// Acquire mutex (see mutex.h for documentation)
//
int Mutex_Acquire(tMutex *Mutex)
{
	tThread	*us = Proc_GetCurThread();
	
	// Get protector
	SHORTLOCK( &Mutex->Protector );
	
	//Log("Mutex_Acquire: (%p)", Mutex);
	
	// Check if the lock is already held
	if( Mutex->Owner ) {
		SHORTLOCK( &glThreadListLock );
		// - Remove from active list
		us = Threads_RemActive();
		us->Next = NULL;
		// - Mark as sleeping
		us->Status = THREAD_STAT_MUTEXSLEEP;
		us->WaitPointer = Mutex;
		
		// - Add to waiting
		if(Mutex->LastWaiting) {
			Mutex->LastWaiting->Next = us;
			Mutex->LastWaiting = us;
		}
		else {
			Mutex->Waiting = us;
			Mutex->LastWaiting = us;
		}
		
		#if DEBUG_TRACE_STATE
		Log("%p (%i %s) waiting on mutex %p",
			us, us->TID, us->ThreadName, Mutex);
		#endif
		
		#if 0
		{
			 int	i = 0;
			tThread	*t;
			for( t = Mutex->Waiting; t; t = t->Next, i++ )
				Log("[%i] (tMutex)%p->Waiting[%i] = %p (%i %s)", us->TID, Mutex, i,
					t, t->TID, t->ThreadName);
		}
		#endif
		
		SHORTREL( &glThreadListLock );
		SHORTREL( &Mutex->Protector );
		while(us->Status == THREAD_STAT_MUTEXSLEEP)	Threads_Yield();
		// We're only woken when we get the lock
		us->WaitPointer = NULL;
	}
	// Ooh, let's take it!
	else {
		Mutex->Owner = us;
		SHORTREL( &Mutex->Protector );
	}
	
	#if 0
	extern tMutex	glPhysAlloc;
	if( Mutex != &glPhysAlloc )
		LogF("Mutex %p taken by %i %p\n", Mutex, us->TID, __builtin_return_address(0));
	#endif
	
	return 0;
}

// Release a mutex
void Mutex_Release(tMutex *Mutex)
{
	SHORTLOCK( &Mutex->Protector );
	//Log("Mutex_Release: (%p)", Mutex);
	if( Mutex->Waiting ) {
		Mutex->Owner = Mutex->Waiting;	// Set owner
		Mutex->Waiting = Mutex->Waiting->Next;	// Next!
		// Reset ->LastWaiting to NULL if we have just removed the last waiting thread
		// 2010-10-02 21:50 - Comemerating the death of the longest single
		//                    blocker in the Acess2 history. REMEMBER TO
		//                    FUCKING MAINTAIN YOUR FUCKING LISTS DIPWIT
		if( Mutex->LastWaiting == Mutex->Owner )
			Mutex->LastWaiting = NULL;
		
		// Wake new owner
		SHORTLOCK( &glThreadListLock );
		if( Mutex->Owner->Status != THREAD_STAT_ACTIVE )
			Threads_AddActive(Mutex->Owner);
		SHORTREL( &glThreadListLock );
	}
	else {
		Mutex->Owner = NULL;
	}
	SHORTREL( &Mutex->Protector );
	
	#if 0
	extern tMutex	glPhysAlloc;
	if( Mutex != &glPhysAlloc )
		LogF("Mutex %p released by %i %p\n", Mutex, Threads_GetTID(), __builtin_return_address(0));
	#endif
}

// Check if a mutex is locked
int Mutex_IsLocked(tMutex *Mutex)
{
	return Mutex->Owner != NULL;
}

// === EXPORTS ===
EXPORT(Mutex_Acquire);
EXPORT(Mutex_Release);
EXPORT(Mutex_IsLocked);
