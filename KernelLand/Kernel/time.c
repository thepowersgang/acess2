/*
 * Acess 2
 * - By John Hodge (thePowersGang) 
 *
 * Timer Code
 */
#include <acess.h>
#include <timers.h>
#include <events.h>
#include <hal_proc.h>	// Proc_GetCurThread

// === CONSTANTS ===
#define	NUM_TIMERS	8

// === TYPEDEFS ===
struct sTimer {
	tTimer	*Next;
	Sint64	FiresAfter;
	void	(*Callback)(void*);
	void	*Argument;
};

// === PROTOTYPES ===
void	Timer_CallTimers(void);

// === GLOBALS ===
volatile Uint64	giTicks = 0;
volatile Sint64	giTimestamp = 0;
volatile Uint64	giPartMiliseconds = 0;
tTimer	*gTimers;	// TODO: Replace by a ring-list timer

// === CODE ===
/**
 * \fn void Timer_CallTimers()
 */
void Timer_CallTimers()
{
	while( gTimers && gTimers->FiresAfter < now() )
	{
		tTimer	*next;
	
		if( gTimers->Callback )
			gTimers->Callback(gTimers->Argument);
		else
			Threads_PostEvent(gTimers->Argument, THREAD_EVENT_TIMER);
		
		next = gTimers->Next;
		free(gTimers);
		gTimers = next;
	}
}

/**
 * \brief Schedule an action
 */
tTimer *Time_CreateTimer(int Delta, tTimerCallback *Callback, void *Argument)
{
	tTimer	*ret;
	tTimer	*t, *p;
	
	if(Callback == NULL)
		Argument = Proc_GetCurThread();

	// TODO: Use a pool instead?
	ret = malloc(sizeof(tTimer));
	
	ret->Callback = Callback;
	ret->FiresAfter = now() + Delta;
	ret->Argument = Argument;

	// Add into list (sorted)
	for( p = (tTimer*)&gTimers, t = gTimers; t; p = t, t = t->Next )
	{
		if( t->FiresAfter > ret->FiresAfter )	break;
	}
	ret->Next = t;
	p->Next = ret;

	return ret;
}

/**
 * \brief Delete a timer
 */
void Time_RemoveTimer(tTimer *Timer)
{
	tTimer	*t, *p;
	for( p = (tTimer*)&gTimers, t = gTimers; t; p = t, t = t->Next )
	{
		if( t == Timer )
		{
			p->Next = t->Next;
			free(Timer);
			return ;
		}
	}
}

/**
 * \fn void Time_Delay(int Delay)
 * \brief Delay for a small ammount of time
 */
void Time_Delay(int Delay)
{
//	tTime	dest = now() + Delay;
//	while(dest > now())	Threads_Yield();
	Time_CreateTimer(Delay, NULL, NULL);
	Threads_WaitEvents(THREAD_EVENT_TIMER);
}

// === EXPORTS ===
EXPORT(Time_CreateTimer);
EXPORT(Time_RemoveTimer);
EXPORT(Time_Delay);
