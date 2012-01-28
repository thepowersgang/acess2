/**
 * Acess2
 * - By John Hodge (thePowersGang)
 *
 * include/hal_proc.h
 * - HAL Process management functions
 * 
 */
#ifndef _HAL_PROC_H_
#define _HAL_PROC_H_
/**
 * \file hal_proc.h
 * \brief Achitecture defined thread/process management functions
 */

#include <threads_int.h>

/**
 * \brief Initialise the architecture dependent side of threading
 */
extern void	ArchThreads_Init(void);
/**
 * \brief Start preemptive multithreading (if needed)
 */
extern void	Proc_Start(void);
/**
 * \brief Called just before a thread is freed
 */
extern void	Proc_ClearThread(tThread *Thread);
/**
 * \brief Called just before a process is freed
 */
extern void	Proc_ClearProcess(tProcess *Process);
/**
 * \brief Get the ID of this CPU
 * \return Zero based CPU ID
 */
extern int	GetCPUNum(void);
/**
 * \brief Create a copy of the current process
 * \param Flags	Options for the clone
 * \return ID of the new thread/process
 */
extern tTID	Proc_Clone(Uint Flags);
/**
 * \brief Create a new kernel thread for the process
 * \param Fnc	Thread root function
 * \param Ptr	Argument to pass the root function
 * \return ID of new thread
 */
extern tTID	Proc_NewKThread( void (*Fnc)(void*), void *Ptr );
/**
 * \brief Start a user task
 * \param Entrypoint	User entrypoint
 * \param Base	Base of executable (argument for ld-acess)
 * \param ArgC	Number of arguments when the program was invoked
 * \param ArgV	Heap allocated arguments and environment (two NULL terminated lists)
 * \param DataSize	Size of the \a ArgV buffer in bytes
 * \note This function should free \a ArgV
 */
extern void	Proc_StartUser(Uint Entrypoint, Uint Base, int ArgC, char **ArgV, int DataSize) NORETURN;
/**
 * \brief Call the fault handler for a thread
 * \param Thread	Thread that is at fault :)
 */
extern void	Proc_CallFaultHandler(tThread *Thread);
/**
 * \brief Dump the CPU state for a thread
 */
extern void	Proc_DumpThreadCPUState(tThread *Thread);
/**
 * \brief Select a new task and run it, suspending this
 */
extern void	Proc_Reschedule(void);

/**
 * \brief Clear the user's memory space back to the minimum required to run
 */
extern void	MM_ClearUser(void);
/**
 * \brief Dump the address space to the debug channel
 * \param Start	First address
 * \param End	Last address
 */
extern void	MM_DumpTables(tVAddr Start, tVAddr End);

/**
 * \brief Check if a buffer is valid (and all user if originally user)
 * \param Addr	Base address
 * \param Size	Size of the buffer in bytes
 * \return Boolean valid (0: invalid, non-0: Valid)
 */
extern int	MM_IsValidBuffer(tVAddr Addr, size_t Size);
#endif
