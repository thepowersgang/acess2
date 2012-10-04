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
extern tThread	*Proc_GetCurThread(void);

extern void	Threads_SetFaultHandler(Uint Handler);

extern int	Threads_SetUID(tUID ID);
extern int	Threads_SetGID(tUID ID);
extern tTID	Threads_WaitTID(int TID, int *Status);


extern int	*Threads_GetMaxFD(void);
extern char	**Threads_GetCWD(void);
extern char	**Threads_GetChroot(void);

extern int	Proc_SendMessage(Uint Dest, int Length, void *Data);
extern int	Proc_GetMessage(Uint *Source, Uint BufSize, void *Buffer);

#endif
