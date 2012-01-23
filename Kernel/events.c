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
void Threads_PostEvent(tThread *Thread, Uint32 EventMask)
{
	// Sanity checking
	if( !Thread )	return ;
	if( EventMask == 0 )	return ;
	// TODO: Check that only one bit is set?
	
	SHORTLOCK( &Thread->IsLocked );

	Thread->EventState |= EventMask;
	
	// Currently sleeping on an event?
	if( Thread->Status == THREAD_STAT_EVENTSLEEP )
	{
		// Waiting on this event?
		if( (Uint32)Thread->RetStatus & EventMask )
		{
			// Wake up
			// TODO: Does it matter if the thread is locked here?
			Threads_AddActive(Thread);
		}
	}
	
	SHORTREL( &Thread->IsLocked );
}

/**
 * \brief Wait for an event to occur
 */
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
	while( !(us->EventState & EventMask) )
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
	
	SHORTREL( &us->IsLocked );
	
	return rv;
}

