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

// Allow nested spinlocks?
#define STACKED_LOCKS	2	// 0: No, 1: Per-CPU, 2: Per-Thread
#define LOCK_DISABLE_INTS	1

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

// === Spinlocks ===
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

// === MACROS ===
/**
 * \brief Halt the CPU
 */
#define	HALT()	__asm__ __volatile__ ("hlt")
/**
 * \brief Fire a magic breakpoint (bochs)
 */
#define	MAGIC_BREAK()	__asm__ __volatile__ ("xchg %bx, %bx")

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
typedef char	BOOL;

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

// === FUNCTIONS ===
extern int	IS_LOCKED(struct sShortSpinlock *Lock);
extern int	CPU_HAS_LOCK(struct sShortSpinlock *Lock);
extern void	SHORTLOCK(struct sShortSpinlock *Lock);
extern void	SHORTREL(struct sShortSpinlock *Lock);

#endif	// !defined(_ARCH_H_)
