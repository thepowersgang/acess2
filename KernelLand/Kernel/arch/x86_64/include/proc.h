/*
 * Acess2 x86_64 Port
 * 
 * proc.h - Process/Thread management code
 */
#ifndef _PROC_H_
#define _PROC_H_

#include <arch.h>

// Register Structure
// TODO: Rebuild once IDT code is done
typedef struct {
	// MMX
	// FPU
	Uint	FS, GS;
	
	Uint	RAX, RCX, RDX, RBX;
	Uint	KernelRSP, RBP, RSI, RDI;
	Uint	R8,  R9,  R10, R11;
	Uint	R12, R13, R14, R15;
	
	Uint	IntNum, ErrorCode;
	Uint	RIP, CS;
	Uint	RFlags, RSP, SS;
} tRegs;

/**
 * \brief Memory State for thread handler
 */
typedef struct sMemoryState
{
	tPAddr	CR3;
}	tMemoryState;

// 512 bytes, 16 byte aligned
typedef struct sSSEState
{
	char	data[512];
} tSSEState;

/**
 * \brief Task state for thread handler
 */
typedef struct sTaskState
{
	Uint	RIP, RSP;
	Uint64	UserRIP, UserCS;
	tSSEState	*SSE;
	 int	bSSEModified;
}	tTaskState;

// === CONSTANTS ===
#define KERNEL_STACK_SIZE	0x8000	// 32 KiB
//#define KERNEL_STACK_SIZE	0x10000	// 64 KiB

#endif

