/*
 * AcessOS Microkernel Version
 * proc.h
 */
#ifndef _PROC_H
#define _PROC_H

#include <threads.h>

// === TYPES ===
#if USE_MP
typedef struct sCPU
{
	Uint8	APICID;
	Uint8	State;	// 0: Unavaliable, 1: Idle, 2: Active
	Uint16	Resvd;
	tThread	*Current;
}	tCPU;
#endif

typedef struct sTSS {
	Uint32	Link;
	Uint32	ESP0, SS0;
	Uint32	ESP1, SS1;
	Uint32	ESP2, SS2;
	Uint32	CR3;
	Uint32	EIP;
	Uint32	EFLAGS;
	Uint32	EAX, ECX, EDX, EBX;
	Uint32	ESP, EBP, ESI, EDI;
	Uint32	ES, CS, DS, SS, FS, GS;
	Uint32	LDTR;
	Uint16	Resvd, IOPB;	// IO Permissions Bitmap
} __attribute__((packed)) tTSS;

// === FUNCTIONS ===
extern void	Proc_Start(void);
extern int	Proc_Clone(Uint *Err, Uint Flags);

#endif
