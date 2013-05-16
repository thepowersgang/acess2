/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * timers.h
 * - Kernel timers
 */
#ifndef _KERNEL_TIMERS_H_
#define _KERNEL_TIMERS_H_
/**
 * \file timers.h
 * \brief Kernel timers
 */

typedef struct sTimer	tTimer;

/**
 * \brief Timer callback function
 */
typedef void (tTimerCallback)(void *);

/**
 * \brief Creates a one-shot timer
 * \param Delta	Period of the timer
 * \param Callback	Function to call each time
 * \param Argument	Argument to pass to the callback
 */
extern tTimer	*Time_CreateTimer(int Delta, tTimerCallback *Callback, void *Argument) DEPRECATED;

/**
 * \brief Allocate (but don't schedule) a timer object
 * \param Callback	Function to call (if NULL, EVENT_TIMER is delivered to the current thread)
 * \param Argument	Argument passed to \a Callback (ignored if \a Callback is NULL)
 * \return New timer pointer
 */
extern tTimer	*Time_AllocateTimer(tTimerCallback *Callback, void *Argument);

/**
 * \brief Free an allocated timer object
 * \param Timer	Object pointer returned by Time_AllocateTimer
 */
extern void	Time_FreeTimer(tTimer *Timer);

/**
 * \brief Schedules a timer to fire in \a Delta ms
 * \param Timer	Timer object returned by Time_AllocateTimer
 * \param Delta	Time until timer fires (in milliseconds)
 */
extern void	Time_ScheduleTimer(tTimer *Timer, int Delta);

/**
 * \brief Removed an active timer
 */
extern void	Time_RemoveTimer(tTimer *Timer);

/**
 * \brief Wait for a period of milliseconds
 */
extern void	Time_Delay(int Delay);

/**
 * \brief Busy wait for a period of milliseconds
 */
extern void	Time_MicroSleep(Uint16 Delay);

#endif

