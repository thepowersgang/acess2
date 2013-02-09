/*
 * Acess2 Kernel (x86 Port)
 * - By John Hodge (thePowersGang)
 * 
 * include/proc_int.h
 * - Low-level threading internals
 */
#ifndef _PROC_INT_H_
#define _PROC_INT_H_

#include <acess.h>
#include <threads.h>
#include <apic.h>

// === TYPES ===
typedef struct sCPU	tCPU;

// === STRUCTS ===
struct sCPU
{
	Uint8	APICID;
	Uint8	State;	// 0: Unavaliable, 1: Idle, 2: Active
	Uint16	Resvd;
	tThread	*Current;
	tThread	*LastTimerThread;	// Used to do preeemption
};

// === FUNCTIONS ===
// - mptable.c - MP Table parsing code
extern const void	*MPTable_LocateFloatPtr(void);
extern int	MPTable_FillCPUs(const void *FloatPtr, tCPU *CPUs, int MaxCPUs, int *BSPIndex);

// - proc.asm
extern void	SwitchTasks(Uint NewSP, Uint *OldSP, Uint NewIP, Uint *OldIO, Uint CR3);
extern void	Proc_InitialiseSSE(void);
extern void	Proc_SaveSSE(Uint DestPtr);
extern void	Proc_DisableSSE(void);

#endif

