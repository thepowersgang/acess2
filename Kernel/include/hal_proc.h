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

#include <threads_int.h>

extern void	ArchThreads_Init(void);
extern void	Proc_Start(void);
extern int	GetCPUNum(void);
extern tTID	Proc_Clone(Uint Flags);
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
extern void	Proc_CallFaultHandler(tThread *Thread);
extern void	Proc_DumpThreadCPUState(tThread *Thread);
extern void	Proc_Reschedule(void);


extern tPAddr	MM_ClearUser(void);
extern void	MM_DumpTables(tVAddr Start, tVAddr End);


#endif
