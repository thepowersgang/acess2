/*
 * Acess2 Kernel
 * mutex.h
 * - Mutual Exclusion syncronisation primitive
 */
#ifndef _MUTEX_H
#define _MUTEX_H

#include <acess.h>

typedef struct sMutex	tMutex;

struct sMutex
{
	tShortSpinlock	Protector;	//!< Protector for the lock strucure
	const char	*Name;	//!< Human-readable name
	struct sThread	*volatile Owner;	//!< Owner of the lock (set upon getting the lock)
	struct sThread	*Waiting;	//!< Waiting threads
	struct sThread	*LastWaiting;	//!< Waiting threads
};

/**
 * \brief Acquire a heavy mutex
 * \param Mutex	Mutex to acquire
 * \return zero on success, -1 if terminated
 * 
 * This type of mutex checks if the mutex is avaliable, and acquires it
 * if it is. Otherwise, the current thread is added to the mutex's wait
 * queue and the thread suspends. When the holder of the mutex completes,
 * the oldest thread (top thread) on the queue is given the lock and
 * restarted.
 */
extern int	Mutex_Acquire(tMutex *Mutex);

/**
 * \brief Release a held mutex
 * \param Mutex	Mutex to release
 * \note Releasing a non-held mutex has no effect
 */
extern void	Mutex_Release(tMutex *Mutex);

/**
 * \brief Is this mutex locked?
 * \param Mutex	Mutex pointer
 */
extern int	Mutex_IsLocked(tMutex *Mutex);

#endif
