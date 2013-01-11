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
 * \brief Acquire a read-write lock for reading
 * \param Lock Lock to acquire
 * \return zero on success, -1 if terminated
 * 
 * Waits until the lock is readable and the increments the reader count
 */
extern int	RWLock_AcquireRead(tRWLock *Lock);

/**
 * \brief Acquire a read-write lock for writing
 * \param Lock Lock to acquire
 * \return zero on success, -1 if terminated
 *
 * Waits until there are no writers, flags all readers to wait, then acquires the lock.
 */
extern int	RWLock_AcquireWrite(tRWLock *Lock);

/**
 * \brief Release a held rw lock
 * \param Lock	Lock to release
 * \note Releasing a non-held lock has no effect
 */
extern void	RWLock_Release(tRWLock *Lock);

#endif
