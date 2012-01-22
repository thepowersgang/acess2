/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * events.c
 * - Thread level event handling
 */
#include <acess.h>
#include <threads_int.h>
#include <events.h>

// === CODE ===
void Threads_PostEvent(tThread *Thread, int EventID)
{
	if( EventID < 0 || EventID > N_EVENTS )
	{
		return ;
	}
	
	SHORTLOCK( Thread->IsLocked );

	Thread->EventState |= 1 << EventID;
	
	// Currently sleeping on an event?
	if( Thread->State == THREAD_STAT_EVENTSLEEP )
	{
		// Waiting on this event?
		if( (Uint32)Thread->RetStatus & (1 << EventID) )
		{
			// Wake up
			Threads_AddActive(Thread);
		}
	}
	
	SHORTREL( Thread->IsLocked );
}

Uint32 Threads_WaitEvents(Uint32 EventMask)
{
	Uint32	rv;
	tThread	*us = Proc_GetCurThread();

	// Early return check
	if( EventMask == 0 )
	{
		return 0;
	}
	
	// Check if a wait is needed
	SHORTLOCK( &us->IsLocked );
	if( !(us->EventState & EventMask) )
	{
		// Wait
		us->RetStatus = EventMask;	// HACK: Store EventMask in RetStatus
		SHORTLOCK( &glThreadListLock );
		us = Threads_RemActive();
		us->Status = THREAD_STAT_EVENTSLEEP;
		SHORTREL( &glThreadListLock );
		SHORTREL( &us->IsLocked );
		while(us->Status == THREAD_STAT_MUTEXSLEEP)	Threads_Yield();
		// Woken when lock is acquired
		SHORTLOCK( &us->IsLocked );
	}
	
	// Get return value and clear changed event bits
	rv = us->EventState & EventMask;
	us->EventState &= ~EventMask;
	
	SHORTREL( us->IsLocked );
	
	return rv;
}

