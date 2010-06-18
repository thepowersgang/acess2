/*
 * Acess2 x86-64 Architecure Module
 * - By John Hodge (thePowersGang)
 */
#ifndef _ARCH_H_
#define _ARCH_H_

#include <stdint.h>
//#define KERNEL_BASE	0xFFFF8000##00000000
#define KERNEL_BASE	0xFFFFFFFF##80000000
#define BITS	64

// === Core Types ===
typedef signed char	Sint8;
typedef unsigned char	Uint8;
typedef signed short	Sint16;
typedef unsigned short	Uint16;
typedef signed int	Sint32;
typedef unsigned int	Uint32;
#if __WORDSIZE == 64
typedef signed long int	Sint64;
typedef unsigned long int	Uint64;
#else
typedef signed long long int	Sint64;
typedef unsigned long long int	Uint64;
#endif

typedef Sint64	Sint;
typedef Uint64	Uint;
typedef Uint64	tPAddr;
typedef Uint64	tVAddr;

typedef Uint64	size_t;

typedef volatile int    tSpinlock;
#define IS_LOCKED(lockptr)      (!!(*(tSpinlock*)lockptr))
#define LOCK(lockptr)   do {int v=1;\
	while(v)\
	__asm__ __volatile__("lock xchgl %0, (%2)":"=r"(v):"r"(1),"r"(lockptr));\
	}while(0)
#define RELEASE(lockptr)	__asm__ __volatile__("lock andl $0, (%0)"::"r"(lockptr));
#define HALT()  __asm__ __volatile__ ("hlt")

// Systemcall Registers
// TODO: Fix this structure
typedef struct sSyscallRegs
{
	union {
		Uint    Num;
		Uint    Return;
	};      // RAX
	Uint    Arg4;	// RCX
	Uint	Arg3;	// RDX
	Uint    Error;	// RBX
	Uint    Resvd1[2];	// Kernel RSP, RBP
	Uint	Arg2;	// RSI
	Uint	Arg1;	// RDI
	Uint	Arg5;	// R8
	Uint	Arg6;	// R9
	Uint	Resvd2[6];	// R10 - R15
	Uint	Resvd3[5];	// IntNum, ErrCode, RIP, CS, RFLAGS
	
	Uint	Resvd4[5];      // Int, Err, rip, CS, ...
	Uint	StackPointer;   // RSP
	Uint	Resvd5[1];      // SS	
}	tSyscallRegs;

#endif

