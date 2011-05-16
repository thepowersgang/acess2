/*
 * Acess2
 * ARM7 Architecture
 *
 * proc.h - Arch-Dependent Process Management
 */
#ifndef _PROC_H_
#define _PROC_H_

#define MAX_CPUS	4

// === STRUCTURES ===
typedef struct {
	Uint32	IP, LR, SP;
	Uint32	UserIP, UserSP;
} tTaskState;

typedef struct {
	Uint32	Base;
} tMemoryState;

typedef struct {
	union {
		Uint32	Num;
		Uint32	Error;
	};
	union {
		Uint32	Arg1;
		Uint32	Return;
	};
	union {
		Uint32	Arg2;
		Uint32	RetHi;
	};
	Uint32	Arg3;
	Uint32	Arg4;
	Uint32	Arg5;
	Uint32	Arg6;	// R6
	Uint32	Unused[13-6];
	Uint32	StackPointer;	// R13
	Uint32	_lr;
	Uint32	_ip;
} tSyscallRegs;

// === MACROS ===
#define HALT()	__asm__ __volatile__ ("nop")

// === PROTOTYPES ===
extern void	Proc_Start(void);
extern tTID	Proc_Clone(Uint *Errno, int Flags);

#endif

