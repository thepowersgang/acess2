/*
 * AcessOS Microkernel Version
 * proc.h
 */
#ifndef _PROC_H
#define _PROC_H

#include <threads.h>

// === CONSTANTS ===
#define GETMSG_IGNORE	((void*)-1)

// === TYPES ===

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
} tTSS;

// === FUNCTIONS ===
extern void	Proc_Start();
extern int	Proc_Clone(Uint *Err, Uint Flags);

#endif
