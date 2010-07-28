/*
 * Acess2
 * - x86 Architecture
 * arch/i386/include/arch.h
 */
#ifndef _ARCH_H_
#define _ARCH_H_

// - Base Defintions
#define	KERNEL_BASE	0xC0000000
#define BITS	32

// - Processor/Machine Specific Features
#if ARCH != i386 && ARCH != i486 && ARCH != i586
# error "Unknown architecture '" #ARCH "'"
#endif

#if USE_MP
# define	MAX_CPUS	8
#else
# define	MAX_CPUS	1
#endif

#if USE_PAE
# define	PHYS_BITS	48
#else
# define	PHYS_BITS	32
#endif

#define __ASM__	__asm__ __volatile__

// === MACROS ===
typedef volatile int	tSpinlock;
#define IS_LOCKED(lockptr)	(!!(*(tSpinlock*)lockptr))
/**
 * \brief Inter-Process interrupt (does a Yield)
 */
#define LOCK(lockptr)	do {\
	int v=1;\
	while(v) {\
		__ASM__("xchgl %%eax, (%%edi)":"=a"(v):"a"(1),"D"(lockptr));\
		if(v) Threads_Yield();\
	}\
}while(0)
/**
 * \brief Tight spinlock (does a HLT)
 */
#define TIGHTLOCK(lockptr)	do{\
	int v=1;\
	while(v) {\
		__ASM__("xchgl %%eax,(%%edi)":"=a"(v):"a"(1),"D"(lockptr));\
		if(v) __ASM__("hlt");\
	}\
}while(0)
/**
 * \brief Very Tight spinlock (short inter-cpu lock)
 */
#define VTIGHTLOCK(lockptr)	do{\
	int v=1;\
	while(v)__ASM__("xchgl %%eax,(%%edi)":"=a"(v):"a"(1),"D"(lockptr));\
}while(0)
/**
 * \brief Release a held spinlock
 */
#define	RELEASE(lockptr)	__ASM__("lock andl $0, (%%edi)"::"D"(lockptr));
/**
 * \brief Halt the CPU
 */
#define	HALT()	__asm__ __volatile__ ("hlt")

// === TYPES ===
typedef unsigned int	Uint;	// Unsigned machine native integer
typedef unsigned char	Uint8;
typedef unsigned short	Uint16;
typedef unsigned long	Uint32;
typedef unsigned long long	Uint64;
typedef signed int		Sint;	// Signed Machine Native integer
typedef signed char		Sint8;
typedef signed short	Sint16;
typedef signed long		Sint32;
typedef signed long long	Sint64;
typedef Uint	size_t;

typedef Uint64	tPAddr;
typedef Uint32	tVAddr;

typedef struct {
    Uint	gs, fs, es, ds;
    Uint	edi, esi, ebp, kesp;
	Uint	ebx, edx, ecx, eax;
    Uint	int_num, err_code;
    Uint	eip, cs;
	Uint	eflags, esp, ss;
} tRegs;

typedef struct {
	Uint	Resvd1[4];	// GS, FS, ES, DS
	Uint	Arg4, Arg5;	// EDI, ESI
	Uint	Arg6;	// EBP
	Uint	Resvd2[1];	// Kernel ESP
	union {
		Uint	Arg1;
		Uint	Error;
	};	// EBX
	union {
		Uint	Arg3;
		Uint	RetHi;	// High 32 bits of ret
	};	// EDX
	Uint	Arg2;	// ECX
	union {
		Uint	Num;
		Uint	Return;
	};	// EAX
	Uint	Resvd3[5];	// Int, Err, Eip, CS, ...
	Uint	StackPointer;	// ESP
	Uint	Resvd4[1];	// SS
} tSyscallRegs;

typedef struct {
	#if USE_PAE
	Uint	PDPT[4];
	#else
	Uint	CR3;
	#endif
} tMemoryState;

typedef struct {
	Uint	EIP, ESP, EBP;
} tTaskState;

#endif	// !defined(_ARCH_H_)
