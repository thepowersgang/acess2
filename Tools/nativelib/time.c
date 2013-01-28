/*
 * Acess2 DiskTool
 * - By John Hodge (thePowersGang)
 * 
 * time.c
 * - Timing functions (emulated)
 */
#include <acess.h>
#include <timers.h>

// === CODE ===
tTimer *Time_AllocateTimer(tTimerCallback *Callback, void *Argument)
{
	return NULL;
}

void Time_ScheduleTimer(tTimer *Timer, int Delta)
{
	if( !Timer )
	{
		// SIGALRM
	}
	else
	{

	}
}

void Time_RemoveTimer(tTimer *Timer)
{
	
}

void Time_FreeTimer(tTimer *Timer)
{
	
}

Sint64 now(void)
{
	// TODO: Translate UNIX time into Acess time
	return 0;
}

