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
extern tTimer	*Time_CreateTimer(int Delta, tTimerCallback *Callback, void *Argument);
/**
 * \brief Removed an active timer
 */
extern void	Time_RemoveTimer(tTimer *Timer);
/**
 * \brief Wait for a period of milliseconds
 */
extern void	Time_Delay(int Delay);

#endif

