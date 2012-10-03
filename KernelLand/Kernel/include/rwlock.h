/*
 * Acess2 Kernel
 * 
 * rwmutex.c
 * - Reader-Writer Mutex (Multiple Readers, One Writer)
 */
#ifndef _RWLOCK_H
#define _RWLOCK_H

#include <acess.h>

typedef struct sRWLock	tRWLock;

struct sRWLock
{
	tShortSpinlock	Protector;	//!< Protector for the lock strucure
	const char	*Name;	//!< Human-readable name
	 int	Level;	// Number of readers active
	struct sThread	*volatile Owner;	//!< Owner of the lock (set upon getting the lock)
	struct sThread	*ReaderWaiting; 	//!< Waiting threads (readers)
	struct sThread	*ReaderWaitingLast;	//!< Waiting threads
	
	struct sThread	*WriterWaiting; 	//!< Waiting threads (writers)
	struct sThread	*WriterWaitingLast;	//!< Waiting threads
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
extern int	RWLock_AcquireRead(tRWLock *Lock);

extern int	RWLock_AcquireWrite(tRWLock *Lock);

/**
 * \brief Release a held mutex
 * \param Mutex	Mutex to release
 * \note Releasing a non-held mutex has no effect
 */
extern void	RWLock_Release(tRWLock *Lock);

#endif
