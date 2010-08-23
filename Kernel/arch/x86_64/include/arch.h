/*
 * Acess2 x86-64 Architecure Module
 * - By John Hodge (thePowersGang)
 */
#ifndef _ARCH_H_
#define _ARCH_H_

//#include <stdint.h>
//#define KERNEL_BASE	0xFFFF8000##00000000
#define KERNEL_BASE	0xFFFFFFFF##80000000
#define BITS	64

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

#define __ASM__	__asm__ __volatile__

// === MACROS ===
/**
 * \brief Halt the CPU
 */
#define	HALT()	__asm__ __volatile__ ("hlt")
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

/**
 * \brief Short Spinlock structure
 */
struct sShortSpinlock {
	volatile int	Lock;	//!< Lock value
	 int	IF;	//!< Interrupt state on call to SHORTLOCK
};
/**
 * \brief Determine if a short spinlock is locked
 * \param Lock	Lock pointer
 */
static inline int IS_LOCKED(struct sShortSpinlock *Lock) {
	return !!Lock->Lock;
}
/**
 * \brief Acquire a Short Spinlock
 * \param Lock	Lock pointer
 * 
 * This type of mutex should only be used for very short sections of code,
 * or in places where a Mutex_* would be overkill, such as appending
 * an element to linked list (usually two assignement lines in C)
 * 
 * \note This type of lock halts interrupts, so ensure that no timing
 * functions are called while it is held.
 */
static inline void SHORTLOCK(struct sShortSpinlock *Lock) {
	 int	v = 1;
	
	// Save interrupt state
	__ASM__ ("pushf;\n\tpop %%rax" : "=a"(Lock->IF));
	Lock->IF &= 0x200;
	
	// Stop interrupts
	__ASM__ ("cli");
	
	// Wait for another CPU to release
	while(v)
		__ASM__("xchgl %%eax, (%%rdi)":"=a"(v):"a"(1),"D"(&Lock->Lock));
}
/**
 * \brief Release a short lock
 * \param Lock	Lock pointer
 */
static inline void SHORTREL(struct sShortSpinlock *Lock) {
	Lock->Lock = 0;
	#if 0	// Which is faster?, meh the test is simpler
	__ASM__ ("pushf;\n\tor %0, (%%rsp);\n\tpopf" : : "a"(Lock->IF));
	#else
	if(Lock->IF)	__ASM__ ("sti");
	#endif
}

#endif

