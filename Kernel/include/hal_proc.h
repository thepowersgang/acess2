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
extern void	Proc_StartUser(Uint Entrypoint, Uint *Bases, int ArgC, char **ArgV, char **EnvP, int DataSize);
extern void	Proc_CallFaultHandler(tThread *Thread);
extern void	Proc_DumpThreadCPUState(tThread *Thread);


extern tPAddr	MM_ClearUser(void);

#endif
