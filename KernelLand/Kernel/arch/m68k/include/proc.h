/*
 * Acess2 M68000 port
 * - By John Hodge (thePowersGang)
 *
 * arch/m68k/include/proc.h
 * - Task management defs
 */
#ifndef _M68K_PROC_H_
#define _M68K_PROC_H_

#define MAX_CPUS	1

typedef int	tMemoryState;	// Unused

typedef struct {
	Uint32	IP;
	Uint32	SP;
} tTaskState;

typedef struct {
	Uint32	Num;
	union {
		Uint32	Arg1;
		Uint32	Return;
	};
	union {
		Uint32	Arg2;
		Uint32	RetHi;
	};
	union {
		Uint32	Arg3;
		Uint32	Error;
	};
	Uint32	Arg4;
	Uint32	Arg5;
	Uint32	Arg6;
} tSyscallRegs;

#endif

