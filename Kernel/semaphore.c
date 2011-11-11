/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * semaphore.c
 * - Semaphores
 */
#include <acess.h>
#include <semaphore.h>
#include <threads_int.h>

#define SEMAPHORE_DEBUG 	0	// Debug semaphores

// === CODE ===
//
// Initialise a semaphore
//
void Semaphore_Init(tSemaphore *Sem, int Value, int MaxValue, const char *Module, const char *Name)
{
	memset(Sem, 0, sizeof(tSemaphore));
	Sem->Value = Value;
	Sem->ModName = Module;
	Sem->Name = Name;
	Sem->MaxValue = MaxValue;
}
//
// Wait for items to be avaliable
//
int Semaphore_Wait(tSemaphore *Sem, int MaxToTake)
{
	tThread	*us;
	 int	taken;
	if( MaxToTake < 0 ) {
		Log_Warning("Threads", "Semaphore_Wait: User bug - MaxToTake(%i) < 0, Sem=%p(%s)",
			MaxToTake, Sem, Sem->Name);
	}
	
	SHORTLOCK( &Sem->Protector );
	
	// Check if there's already items avaliable
	if( Sem->Value > 0 ) {
		// Take what we need
		if( MaxToTake && Sem->Value > MaxToTake )
			taken = MaxToTake;
		else
			taken = Sem->Value;
		Sem->Value -= taken;
	}
	else
	{
		SHORTLOCK( &glThreadListLock );
		
		// - Remove from active list
		us = Threads_RemActive();
		us->Next = NULL;
		// - Mark as sleeping
		us->Status = THREAD_STAT_SEMAPHORESLEEP;
		us->WaitPointer = Sem;
		us->RetStatus = MaxToTake;	// Use RetStatus as a temp variable
		
		// - Add to waiting
		if(Sem->LastWaiting) {
			Sem->LastWaiting->Next = us;
			Sem->LastWaiting = us;
		}
		else {
			Sem->Waiting = us;
			Sem->LastWaiting = us;
		}
		
		#if DEBUG_TRACE_STATE || SEMAPHORE_DEBUG
		Log("%p (%i %s) waiting on semaphore %p %s:%s",
			us, us->TID, us->ThreadName,
			Sem, Sem->ModName, Sem->Name);
		#endif
		
		SHORTREL( &Sem->Protector );	// Release first to make sure it is released
		SHORTREL( &glThreadListLock );
		while( us->Status == THREAD_STAT_SEMAPHORESLEEP )
		{
			Threads_Yield();
			if(us->Status == THREAD_STAT_SEMAPHORESLEEP)
				Log_Warning("Threads", "Semaphore %p %s:%s re-schedulued while asleep",
					Sem, Sem->ModName, Sem->Name);
		}
		#if DEBUG_TRACE_STATE || SEMAPHORE_DEBUG
		Log("Semaphore %p %s:%s woken", Sem, Sem->ModName, Sem->Name);
		#endif
		// We're only woken when there's something avaliable (or a signal arrives)
		us->WaitPointer = NULL;
		
		taken = us->RetStatus;
		
		// Get the lock again
		SHORTLOCK( &Sem->Protector );
	}
	
	// While there is space, and there are thread waiting
	// wake the first thread and give it what it wants (or what's left)
	while( (Sem->MaxValue == 0 || Sem->Value < Sem->MaxValue) && Sem->Signaling )
	{
		 int	given;
		tThread	*toWake = Sem->Signaling;
		
		Sem->Signaling = Sem->Signaling->Next;
		// Reset ->LastWaiting to NULL if we have just removed the last waiting thread
		if( Sem->Signaling == NULL )
			Sem->LastSignaling = NULL;
		
		// Figure out how much to give
		if( toWake->RetStatus && Sem->Value + toWake->RetStatus < Sem->MaxValue )
			given = toWake->RetStatus;
		else
			given = Sem->MaxValue - Sem->Value;
		Sem->Value -= given;
		
		
		#if DEBUG_TRACE_STATE || SEMAPHORE_DEBUG
		Log("%p (%i %s) woken by wait on %p %s:%s",
			toWake, toWake->TID, toWake->ThreadName,
			Sem, Sem->ModName, Sem->Name);
		#endif
		
		// Save the number we gave to the thread's status
		toWake->RetStatus = given;
		
		// Wake the sleeper
		SHORTLOCK( &glThreadListLock );
		if( toWake->Status != THREAD_STAT_ACTIVE )
			Threads_AddActive(toWake);
		SHORTREL( &glThreadListLock );
	}
	SHORTREL( &Sem->Protector );
	
	#if DEBUG_TRACE_STATE || SEMAPHORE_DEBUG
	Log("Semaphore %p %s:%s took %i by wait",
		Sem, Sem->ModName, Sem->Name, taken);
	#endif

	return taken;
}

//
// Add items to a semaphore
//
int Semaphore_Signal(tSemaphore *Sem, int AmmountToAdd)
{
	 int	given;
	 int	added;
	
	if( AmmountToAdd < 0 ) {
		Log_Warning("Threads", "Semaphore_Signal: User bug - AmmountToAdd(%i) < 0, Sem=%p(%s)",
			AmmountToAdd, Sem, Sem->Name);
	}
	SHORTLOCK( &Sem->Protector );
	
	// Check if we have to block
	if( Sem->MaxValue && Sem->Value == Sem->MaxValue )
	{
		tThread	*us;
		#if 0
		Log_Debug("Threads", "Semaphore_Signal: IDLE Sem = %s:%s", Sem->ModName, Sem->Name);
		Log_Debug("Threads", "Semaphore_Signal: Sem->Value(%i) == Sem->MaxValue(%i)", Sem->Value, Sem->MaxValue);
		#endif
		
		SHORTLOCK( &glThreadListLock );
		// - Remove from active list
		us = Threads_RemActive();
		us->Next = NULL;
		// - Mark as sleeping
		us->Status = THREAD_STAT_SEMAPHORESLEEP;
		us->WaitPointer = Sem;
		us->RetStatus = AmmountToAdd;	// Use RetStatus as a temp variable
		
		// - Add to waiting
		if(Sem->LastSignaling) {
			Sem->LastSignaling->Next = us;
			Sem->LastSignaling = us;
		}
		else {
			Sem->Signaling = us;
			Sem->LastSignaling = us;
		}
		
		#if DEBUG_TRACE_STATE || SEMAPHORE_DEBUG
		Log("%p (%i %s) signaling semaphore %p %s:%s",
			us, us->TID, us->ThreadName,
			Sem, Sem->ModName, Sem->Name);
		#endif
		
		SHORTREL( &glThreadListLock );	
		SHORTREL( &Sem->Protector );
		while(us->Status == THREAD_STAT_SEMAPHORESLEEP)	Threads_Yield();
		// We're only woken when there's something avaliable
		us->WaitPointer = NULL;
		
		added = us->RetStatus;
		
		// Get the lock again
		SHORTLOCK( &Sem->Protector );
	}
	// Non blocking
	else
	{
		// Figure out how much we need to take off
		if( Sem->MaxValue && Sem->Value + AmmountToAdd > Sem->MaxValue)
			added = Sem->MaxValue - Sem->Value;
		else
			added = AmmountToAdd;
		Sem->Value += added;
	}
	
	// While there are items avaliable, and there are thread waiting
	// wake the first thread and give it what it wants (or what's left)
	while( Sem->Value && Sem->Waiting )
	{
		tThread	*toWake = Sem->Waiting;
		
		// Remove thread from list (double ended, so clear LastWaiting if needed)
		Sem->Waiting = Sem->Waiting->Next;
		if( Sem->Waiting == NULL )
			Sem->LastWaiting = NULL;
		
		// Figure out how much to give to woken thread
		// - Requested count is stored in ->RetStatus
		if( toWake->RetStatus && Sem->Value > toWake->RetStatus )
			given = toWake->RetStatus;
		else
			given = Sem->Value;
		Sem->Value -= given;
		
		// Save the number we gave to the thread's status
		toWake->RetStatus = given;
		
		if(toWake->bInstrTrace)
			Log("%s(%i) given %i from %p", toWake->ThreadName, toWake->TID, given, Sem);
		#if DEBUG_TRACE_STATE || SEMAPHORE_DEBUG
		Log("%p (%i %s) woken by signal on %p %s:%s",
			toWake, toWake->TID, toWake->ThreadName,
			Sem, Sem->ModName, Sem->Name);
		#endif
		
		// Wake the sleeper
		if( toWake->Status != THREAD_STAT_ACTIVE )
			Threads_AddActive(toWake);
		else
			Warning("Thread %p (%i %s) is already awake", toWake, toWake->TID, toWake->ThreadName);
	}
	SHORTREL( &Sem->Protector );
	
	return added;
}

//
// Get the current value of a semaphore
//
int Semaphore_GetValue(tSemaphore *Sem)
{
	return Sem->Value;
}

// === EXPORTS ===
EXPORT(Semaphore_Init);
EXPORT(Semaphore_Wait);
EXPORT(Semaphore_Signal);
