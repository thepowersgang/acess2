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
	
//	Log("Mutex_Acquire: (%p)", Mutex);
	
	// Check if the lock is already held
	if( Mutex->Owner ) {
		// Sleep on the lock
		Threads_int_Sleep(THREAD_STAT_MUTEXSLEEP,
			Mutex, 0,
			&Mutex->Waiting, &Mutex->LastWaiting, &Mutex->Protector);
		// - We're only woken when we get the lock
	}
	else {
		// If not, just obtain it
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
		if( Mutex->LastWaiting == Mutex->Owner )
			Mutex->LastWaiting = NULL;
		
		// Wake new owner
		if( Mutex->Owner->Status != THREAD_STAT_ACTIVE )
			Threads_AddActive(Mutex->Owner);
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
