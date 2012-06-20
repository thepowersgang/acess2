/*
 * Acess2 x86-64 Architecure Module
 * - By John Hodge (thePowersGang)
 */
#ifndef _ARCH_H_
#define _ARCH_H_

//#include <stdint.h>
#define	USER_MAX 	0x00007FFF##FFFFF000
#define KERNEL_BASE	0xFFFFFFFF##80000000
#define BITS	64
#define PAGE_SIZE	0x1000

#ifndef MAX_CPUS
# define MAX_CPUS	8
#endif

#define STACKED_LOCKS	2	// 0: No, 1: Per-CPU, 2: Per-Thread
#define LOCK_DISABLE_INTS	0

#define INVLPTR	((void*)0x0FFFFFFFFFFFFFFFULL)

//#define INT_MAX	0x7FFFFFFF
//#define UINT_MAX	0xFFFFFFFF

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
typedef char	BOOL;

#define __ASM__	__asm__ __volatile__

// === MACROS ===
/**
 * \brief Halt the CPU
 */
#define	HALT()	__asm__ __volatile__ ("sti;\n\thlt")
/**
 * \brief Fire a magic breakpoint (bochs)
 */
#define	MAGIC_BREAK()	__asm__ __volatile__ ("xchg %bx, %bx")

// Systemcall Registers
// TODO: Fix this structure
typedef struct sSyscallRegs
{
	union {
		Uint    Num;
		Uint    Return;
	};      // RAX
	Uint    Error;	// RBX
	Uint	Arg1;	// RDI
	Uint	Arg2;	// RSI
	Uint	Arg3;	// RDX
	Uint    Arg4;	// RCX
	Uint	Arg5;	// R8
	Uint	Arg6;	// R9
	Uint	_Flags;
	Uint	_IP;
	Uint	StackPointer;   // RSP
	
}	tSyscallRegs;

/**
 * \brief Short Spinlock structure
 */
struct sShortSpinlock {
	#if STACKED_LOCKS == 2
	volatile void	*Lock;	//!< Lock value
	#else
	volatile int	Lock;	//!< Lock value
	#endif
	
	#if LOCK_DISABLE_INTS
	 int	IF;	//!< Interrupt state on call to SHORTLOCK
	#endif
	#if STACKED_LOCKS
	 int	Depth;
	#endif
};

// === FUNCTIONS ===
extern int	IS_LOCKED(struct sShortSpinlock *Lock);
extern int	CPU_HAS_LOCK(struct sShortSpinlock *Lock);
extern void	SHORTLOCK(struct sShortSpinlock *Lock);
extern void	SHORTREL(struct sShortSpinlock *Lock);

extern void	Debug_PutCharDebug(char ch);
extern void	Debug_PutStringDebug(const char *Str);

// TODO: Move this to acess.h
extern tPAddr	MM_AllocateZero(tVAddr VAddr);

#endif

