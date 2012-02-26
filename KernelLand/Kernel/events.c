/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * events.c
 * - Thread level event handling
 */
#define DEBUG	0
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
	
	ENTER("pThread xEventMask", Thread, EventMask);

	SHORTLOCK( &Thread->IsLocked );

	Thread->EventState |= EventMask;
	LOG("Thread->EventState = 0x%x", Thread->EventState);
	
	// Currently sleeping on an event?
	if( Thread->Status == THREAD_STAT_EVENTSLEEP )
	{
		// Waiting on this event?
		if( (Uint32)Thread->RetStatus & EventMask )
		{
			// Wake up
			LOG("Waking thread %p(%i %s)", Thread, Thread->TID, Thread->ThreadName);
			Threads_AddActive(Thread);
		}
	}
	
	SHORTREL( &Thread->IsLocked );
	LEAVE('-');
}

/**
 * \brief Clear an event without waiting
 */
void Threads_ClearEvent(Uint32 EventMask)
{
	Proc_GetCurThread()->EventState &= ~EventMask;
}

/**
 * \brief Wait for an event to occur
 */
Uint32 Threads_WaitEvents(Uint32 EventMask)
{
	Uint32	rv;
	tThread	*us = Proc_GetCurThread();

	ENTER("xEventMask", EventMask);

	// Early return check
	if( EventMask == 0 )
	{
		LEAVE('i', 0);
		return 0;
	}
	
	LOG("us = %p(%i %s)", us, us->TID, us->ThreadName);

	// Check if a wait is needed
	SHORTLOCK( &us->IsLocked );
	while( !(us->EventState & EventMask) )
	{
		LOG("Locked and preparing for wait");
		// Wait
		us->RetStatus = EventMask;	// HACK: Store EventMask in RetStatus
		SHORTLOCK( &glThreadListLock );
		us = Threads_RemActive();
		us->Status = THREAD_STAT_EVENTSLEEP;
		// Note stored anywhere because we're woken using other means
		SHORTREL( &glThreadListLock );
		SHORTREL( &us->IsLocked );
		while(us->Status == THREAD_STAT_EVENTSLEEP)	Threads_Yield();
		// Woken when lock is acquired
		SHORTLOCK( &us->IsLocked );
	}
	
	// Get return value and clear changed event bits
	rv = us->EventState & EventMask;
	us->EventState &= ~EventMask;
	
	SHORTREL( &us->IsLocked );
	
	LEAVE('x', rv);
	return rv;
}

