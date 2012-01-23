/*
 * Acess2 Kernel
 */
#ifndef _THREADS_H_
#define _THREADS_H_

#include <arch.h>
#include <signal.h>
//#include <proc.h>

enum eFaultNumbers
{
	FAULT_MISC,
	FAULT_PAGE,
	FAULT_ACCESS,
	FAULT_DIV0,
	FAULT_OPCODE,
	FAULT_FLOAT
};

#define GETMSG_IGNORE	((void*)-1)

typedef struct sThread	tThread;

// === FUNCTIONS ===
extern void	Threads_SetFaultHandler(Uint Handler);

extern int	Threads_SetUID(Uint *Errno, tUID ID);
extern int	Threads_SetGID(Uint *Errno, tUID ID);
extern int	Threads_WaitTID(int TID, int *Status);

extern int	Proc_SendMessage(Uint *Err, Uint Dest, int Length, void *Data);
extern int	Proc_GetMessage(Uint *Err, Uint *Source, void *Buffer);

#endif
