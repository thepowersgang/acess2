/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * time.c
 * - Timer code
 */
#include <acess.h>
#include <timers.h>

struct sTimer {
	tTimer	*Next;
	Sint64	FiresAfter;
	void	(*Callback)(void*);
	void	*Argument;
	BOOL	bActive;
};

// === CODE ===
tTimer *Time_CreateTimer(int Delta, tTimerCallback *Callback, void *Argument)
{
	return NULL;
}

tTimer *Time_AllocateTimer(tTimerCallback *Callback, void *Argument)
{
	return NULL;
}

void Time_FreeTimer(tTimer *Timer)
{
}

void Time_ScheduleTimer(tTimer *Timer, int Delta)
{
}

void Time_RemoveTimer(tTimer *Timer)
{
}
