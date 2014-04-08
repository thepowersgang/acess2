/*
 * Acess2 DiskTool
 * - By John Hodge (thePowersGang)
 * 
 * time.c
 * - Timing functions (emulated)
 */
#include <acess.h>
#include <timers.h>
#include <events.h>

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
		Log_Warning("Time", "TODO: Alarm event in %i ms", Delta);
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

void Time_ScheduleEvent(int Delay)
{
	Log_Warning("Time", "TODO: EVENT_TIMER in %ims", Delay);
}

void Time_Delay(int Delay)
{
	Time_ScheduleEvent(Delay);
	Threads_WaitEvents(THREAD_EVENT_TIMER);
}

Sint64 now(void)
{
	// TODO: Translate UNIX time into Acess time
	return 0;
}

