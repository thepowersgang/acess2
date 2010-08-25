/*
 * Acess 2
 * - By John Hodge (thePowersGang) 
 *
 * Timer Code
 */
#include <acess.h>

// === CONSTANTS ===
#define	NUM_TIMERS	8

// === TYPEDEFS ===
typedef struct sTimer {
	 int	FiresAfter;
	void	(*Callback)(void*);
	void	*Argument;
} tTimer;

// === PROTOTYPES ===
Sint64	now(void);
void	Timer_CallTimers(void);

// === GLOBALS ===
Uint64	giTicks = 0;
Sint64	giTimestamp = 0;
Uint64	giPartMiliseconds = 0;
tTimer	gTimers[NUM_TIMERS];	// TODO: Replace by a ring-list timer

// === CODE ===
/**
 * \fn Sint64 now()
 * \brief Return the current timestamp
 */
Sint64 now(void)
{
	return giTimestamp;
}

/**
 * \fn void Timer_CallTimers()
 */
void Timer_CallTimers()
{
	 int	i;
	void	(*callback)(void *);
	void	*arg;
	
	for(i = 0; i < NUM_TIMERS; i ++)
	{
		if(gTimers[i].Callback == NULL)	continue;
		if(giTimestamp < gTimers[i].FiresAfter)	continue;
		callback = gTimers[i].Callback;	arg = gTimers[i].Argument;
		gTimers[i].Callback = NULL;
		callback(arg);
	}
}

/**
 * \fn int Time_CreateTimer(int Delta, tTimerCallback *Callback, void *Argument)
 */
int Time_CreateTimer(int Delta, tTimerCallback *Callback, void *Argument)
{
	 int	ret;
	
	if(Callback == NULL)	return -1;
	
	for(ret = 0;
		ret < NUM_TIMERS;
		ret++)
	{
		if(gTimers[ret].Callback != NULL)	continue;
		gTimers[ret].Callback = Callback;
		gTimers[ret].FiresAfter = giTimestamp + Delta;
		gTimers[ret].Argument = Argument;
		//Log("Callback = %p", Callback);
		//Log("Timer %i fires at %lli", ret, gTimers[ret].FiresAfter);
		return ret;
	}
	return -1;
}

/**
 * \fn void Time_RemoveTimer(int ID)
 */
void Time_RemoveTimer(int ID)
{
	if(ID < 0 || ID >= NUM_TIMERS)	return;
	gTimers[ID].Callback = NULL;
}

/**
 * \fn void Time_Delay(int Delay)
 * \brief Delay for a small ammount of time
 */
void Time_Delay(int Delay)
{
	Sint64	dest = giTimestamp + Delay;
	while(dest > giTimestamp)	Threads_Yield();
}

// === EXPORTS ===
EXPORT(now);
EXPORT(Time_CreateTimer);
EXPORT(Time_RemoveTimer);
EXPORT(Time_Delay);
