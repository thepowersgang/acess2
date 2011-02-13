/*
 * Acess2 Kernel
 * semaphore.h
 * - Semaphore syncronisation primitive
 */
#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include <acess.h>

/**
 * \brief Semaphore typedef
 */
typedef struct sSemaphore	tSemaphore;

/**
 * \brief Semaphore structure
 */
struct sSemaphore {
	tShortSpinlock	Protector;	//!< Protector for the lock strucure
	const char	*ModName;	//!< Human-readable module name
	const char	*Name;	//!< Human-readable name
	volatile int	Value;	//!< Current value
	volatile int	MaxValue;	//!< Maximum value (signal will wait if it will go over this)
	
	struct sThread	*Waiting;	//!< Waiting threads
	struct sThread	*LastWaiting;	//!< Waiting threads
	struct sThread	*Signaling;	//!< Waiting threads (from Semaphore_Signal)
	struct sThread	*LastSignaling;	//!< Last waiting thread (from Semaphore_Signal)
};

/**
 * \brief Initialise the semaphore
 * \param Sem	Semaphore structure to initialsie
 * \param Value	Initial value of the semaphore
 * \param Module	Module name
 * \param Name	Symbolic name
 * \note Not always needed, as initialising to 0 is valid, but it is preferred
 *       if all semaphores have \a Name set
 * 
 * \note \a Module and \a Name must be avaliable for as long as the semaphore is used
 */
extern void	Semaphore_Init(tSemaphore *Sem, int InitValue, int MaxValue, const char *Module, const char *Name);
/**
 * \brief Acquire items from the semaphore
 * \param Semaphore	Semaphore structure to use
 * \param MaxToTake	Maximum number of items to take off the list (if zero, as much as possible is taken)
 * \return Number of items fetched
 * \retval 0	Semaphore interrupted
 * \retval -1	Unspecified error
 */
extern int	Semaphore_Wait(tSemaphore *Sem, int MaxToTake);
/**
 * \brief Add an "item" to the semaphore
 * \param Sem	Semaphore to use
 * \param AmmountToAdd	Number of items to add
 * \return Actual number of items added
 * \retval 0	Semaphore interrupted
 * \retval -1	Unspecified error
 */
extern int	Semaphore_Signal(tSemaphore *Sem, int AmmountToAdd);
/**
 * \brief Get the number of items waiting in a semaphore
 * \param Sem	Semaphore to use
 * \return Number of waiting items
 */
extern int	Semaphore_GetValue(tSemaphore *Sem);

#endif
