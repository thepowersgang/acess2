/*
 * Acess2 x86-64 Architecure Module
 * - By John Hodge (thePowersGang)
 */
#ifndef _ARCH_H_
#define _ARCH_H_

#include <stdint.h>
#define KERNEL_BASE	0xFFFF8000##00000000
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

typedef Uint64	Uint;
typedef Uint64	tPAddr;
typedef Uint64	tVAddr;

//typedef unsigned int	size_t;
typedef Uint64	size_t;

typedef volatile int    tSpinlock;
#define IS_LOCKED(lockptr)      (!!(*(tSpinlock*)lockptr))
#define LOCK(lockptr)   do {int v=1;\
	while(v)\
	__asm__ __volatile__("lock xchgl %%eax, (%%edi)":"=a"(v):"a"(1),"D"(lockptr));\
	}while(0)
#define RELEASE(lockptr)	__asm__ __volatile__("lock andl $0, (%%edi)"::"D"(lockptr));
#define HALT()  __asm__ __volatile__ ("hlt")

// Systemcall Registers
typedef struct sSyscallRegs
{
	Uint    Arg4, Arg5;     // RDI, RSI
	Uint    Arg6;   // RBP
	Uint    Resvd2[1];      // Kernel RSP
	union {
	        Uint    Arg1;
	        Uint    Error;
	};      // RBX
	union {
	        Uint    Arg3;
	        Uint    RetHi;  // High 64 bits of ret
	};      // RDX
	Uint    Arg2;   // RCX
	union {
	        Uint    Num;
	        Uint    Return;
	};      // RAX
	Uint    Resvd3[5];      // Int, Err, rip, CS, ...
	Uint    StackPointer;   // RSP
	Uint    Resvd4[1];      // SS	
}	tSyscallRegs;

#endif

