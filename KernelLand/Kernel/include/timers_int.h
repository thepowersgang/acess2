/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * include/timers_int.h
 * - Timer internal header
 * - Only for use by code that needs access to timer internals
 */
#ifndef _KERNEL__TIMERS_INT_H_
#define _KERNEL__TIMERS_INT_H_

#include <timers.h>

// === TYPEDEFS ===
struct sTimer {
	tTimer	*Next;
	Sint64	FiresAfter;
	void	(*Callback)(void*);
	void	*Argument;
//	tMutex	Lock;
	BOOL	bActive;
};

#endif

