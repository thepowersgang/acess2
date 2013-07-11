/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang) 
 *
 * Timer Code
 */
#define SANITY	1	// Enable ASSERTs
#define DEBUG	0
#include <acess.h>
#include <timers.h>
#include <timers_int.h>
#include <events.h>
#include <hal_proc.h>	// Proc_GetCurThread
#include <workqueue.h>
#include <threads_int.h>	// Used to get thread timer

// === PROTOTYPES ===
void	Timer_CallbackThread(void *Unused);
void	Timer_CallTimers(void);
#if 0
tTimer	*Time_CreateTimer(int Delta, tTimerCallback *Callback, void *Argument);
void	Time_ScheduleTimer(tTimer *Timer, int Delta);
void	Time_RemoveTimer(tTimer *Timer);
tTimer	*Time_AllocateTimer(tTimerCallback *Callback, void *Argument);
#endif
void	Time_InitTimer(tTimer *Timer, tTimerCallback *Callback, void *Argument);
#if 0
void	Time_FreeTimer(tTimer *Timer);
void	Time_Delay(int Time);
#endif

// === GLOBALS ===
volatile Uint64	giTicks = 0;
volatile Sint64	giTimestamp = 0;
volatile Uint64	giPartMiliseconds = 0;
tTimer	*gTimers;
tWorkqueue	gTimers_CallbackQueue;
tShortSpinlock	gTimers_ListLock;

// === CODE ===
void Timer_CallbackThread(void *Unused)
{
	Threads_SetName("Timer Callback Thread");
	Workqueue_Init(&gTimers_CallbackQueue, "Timer Callbacks", offsetof(tTimer, Next));

	for(;;)
	{
		tTimer *timer = Workqueue_GetWork(&gTimers_CallbackQueue);
	
		if( !timer->Callback ) {
			LOG("Timer %p doesn't have a callback", timer);
			ASSERT( timer->Callback );
		}

		// Save callback and argument (because once the mutex is released
		// the timer may no longer be valid)
		tTimerCallback	*cb = timer->Callback;
		void	*arg = timer->Argument;
		
		LOG("Callback fire %p", timer);
	
		// Allow Time_RemoveTimer to be called safely
		timer->bActive = 0;
		
		// Fire callback
		cb(arg);

		// Mark timer as no longer needed
	}
}

/**
 * \fn void Timer_CallTimers()
 */
void Timer_CallTimers()
{
	SHORTLOCK(&gTimers_ListLock);
	while( gTimers && gTimers->FiresAfter < now() )
	{
		// Get timer from list
		tTimer	*timer = gTimers;
	
		ASSERT( gTimers != gTimers->Next );	
		gTimers = gTimers->Next;
	
		// Perform event
		if( timer->Callback ) {
			LOG("Callback schedule %p", timer);
			// PROBLEM! Possibly causes rescheudle during interrupt
//			Mutex_Acquire( &timer->Lock );	// Released once the callback fires
			Workqueue_AddWork(&gTimers_CallbackQueue, timer);
		}
		else {
			LOG("Event fire %p", timer);
			ASSERT( timer->Argument );
			Threads_PostEvent(timer->Argument, THREAD_EVENT_TIMER);
			timer->bActive = 0;
		}
	}
	SHORTREL(&gTimers_ListLock);
}

/**
 * \brief Schedule an action (Legacy)
 */
tTimer *Time_CreateTimer(int Delta, tTimerCallback *Callback, void *Argument)
{
	tTimer	*ret = malloc(sizeof(tTimer));
	if( !ret )	return NULL;
	Time_InitTimer(ret, Callback, Argument);
	Time_ScheduleTimer(ret, Delta);
	return ret;
}

/**
 * \brief Schedule a timer to fire
 */
void Time_ScheduleTimer(tTimer *Timer, int Delta)
{
	tTimer	*t;
	tTimer	**prev_next;

	// Sanity checks
	if( !Timer )	return ;

	if( Timer->bActive )	return;
	
	// Set time
	Timer->FiresAfter = now() + Delta;
	
	// Debug
	LOG("%p added timer %p - %i ms (ts=%lli)",
		__builtin_return_address(0), Timer, Delta, Timer->FiresAfter);

	// Add into list (sorted)
	SHORTLOCK(&gTimers_ListLock);
//	Mutex_Release( &Timer->Lock );	// Prevent deadlocks
	for( prev_next = &gTimers, t = gTimers; t; prev_next = &t->Next, t = t->Next )
	{
		ASSERTC( *prev_next, !=, t );
		ASSERT( CheckMem(t, sizeof(tTimer)) );
		if( t == Timer )
		{
			LOG("Double schedule - increasing delta");
			SHORTREL(&gTimers_ListLock);
			return ;
		}
		LOG(" t = %p ts:%lli", t, t->FiresAfter);
		if( t->FiresAfter > Timer->FiresAfter )	break;
	}
	Timer->Next = t;
	*prev_next = Timer;
	Timer->bActive = 1;
	LOG(" %p %p %p", prev_next, Timer, t);
	SHORTREL(&gTimers_ListLock);
}

/**
 * \brief Delete a timer from the running list
 */
void Time_RemoveTimer(tTimer *Timer)
{
	tTimer	*t;
	tTimer	**prev_next;

	if( !Timer )	return ;
	
	SHORTLOCK(&gTimers_ListLock);
	for( prev_next = &gTimers, t = gTimers; t; prev_next = &t->Next, t = t->Next )
	{
		ASSERT( *prev_next != t ); ASSERT( CheckMem(t, sizeof(tTimer)) );
		if( t == Timer )
		{
			*prev_next = t->Next;
			break ;
		}
	}
	SHORTREL(&gTimers_ListLock);

	if( t ) {
		Timer->bActive = 0;
		LOG("%p removed %p", __builtin_return_address(0), Timer);
	}
	else
		LOG("%p tried to remove %p (already complete)", __builtin_return_address(0), Timer);
}

/**
 * \brief Allocate a timer object, but don't schedule it
 */
tTimer *Time_AllocateTimer(tTimerCallback *Callback, void *Argument)
{
	tTimer	*ret = malloc(sizeof(tTimer));
	if( !ret )	return NULL;
	Time_InitTimer(ret, Callback, Argument);
	return ret;
}

/**
 * \brief Initialise a timer
 * \note Mostly an internal function
 */
void Time_InitTimer(tTimer *Timer, tTimerCallback *Callback, void *Argument)
{
	if(Callback == NULL)
		Argument = Proc_GetCurThread();
	Timer->FiresAfter = 0;
	Timer->Callback = Callback;
	Timer->Argument = Argument;
//	memset( &Timer->Lock, 0, sizeof(Timer->Lock) );
	Timer->bActive = 0;
}

/**
 * \brief Free an allocated timer
 */
void Time_FreeTimer(tTimer *Timer)
{
	if( !Timer )	return ;
	Time_RemoveTimer( Timer );	// Just in case

	// Ensures that we don't free until the timer callback has started
	while( Timer->bActive )	Threads_Yield();
	// Release won't be needed, as nothing should be waiting on it
//	Mutex_Acquire( &Timer->Lock );
	
	// Free timer
	free(Timer);
	LOG("%p deallocated %p", __builtin_return_address(0), Timer);
}

/**
 * \fn void Time_Delay(int Delay)
 * \brief Delay for a small ammount of time
 */
void Time_Delay(int Delay)
{
	tTimer	*t = &Proc_GetCurThread()->ThreadTimer;
	Time_InitTimer(t, NULL, NULL);
	Time_ScheduleTimer(t, Delay);
	Threads_WaitEvents(THREAD_EVENT_TIMER);
}

// === EXPORTS ===
//EXPORT(Time_CreateTimer);
EXPORT(Time_ScheduleTimer);
EXPORT(Time_RemoveTimer);
EXPORT(Time_AllocateTimer);
//EXPORT(Time_InitTimer);
EXPORT(Time_FreeTimer);
EXPORT(Time_Delay);
