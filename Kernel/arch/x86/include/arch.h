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
#define STACKED_LOCKS	1
#define LOCK_DISABLE_INTS	0

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
	volatile int	Lock;	//!< Lock value
	#if LOCK_DISABLE_INTS
	 int	IF;	//!< Interrupt state on call to SHORTLOCK
	#endif
	#if STACKED_LOCKS
	 int	Depth;
	#endif
};
/**
 * \brief Determine if a short spinlock is locked
 * \param Lock	Lock pointer
 */
static inline int IS_LOCKED(struct sShortSpinlock *Lock) {
	return !!Lock->Lock;
}

/**
 * \brief Check if the current CPU has the lock
 * \param Lock	Lock pointer
 */
static inline int CPU_HAS_LOCK(struct sShortSpinlock *Lock) {
	extern int	GetCPUNum(void);
	return Lock->Lock == GetCPUNum() + 1;
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
 * functions are called while it is held. As a matter of fact, spend as
 * little time as possible with this lock held
 */
static inline void SHORTLOCK(struct sShortSpinlock *Lock) {
	 int	v = 1;
	#if LOCK_DISABLE_INTS
	 int	IF;
	#endif
	#if STACKED_LOCKS
	extern int	GetCPUNum(void);
	 int	cpu = GetCPUNum() + 1;
	#endif
	
	#if LOCK_DISABLE_INTS
	// Save interrupt state and clear interrupts
	__ASM__ ("pushf;\n\tpop %%eax\n\tcli" : "=a"(IF));
	IF &= 0x200;	// AND out all but the interrupt flag
	#endif
	
	#if STACKED_LOCKS
	if( Lock->Lock == cpu ) {
		Lock->Depth ++;
		return ;
	}
	#endif
	
	// Wait for another CPU to release
	while(v) {
		#if STACKED_LOCKS
		// CMPXCHG:
		//  If r/m32 == EAX, set ZF and set r/m32 = r32
		//  Else, clear ZF and set EAX = r/m32
		__ASM__("lock cmpxchgl %2, (%3)"
			: "=a"(v)
			: "a"(0), "r"(cpu), "r"(&Lock->Lock)
			);
		#else
		__ASM__("xchgl %%eax, (%%edi)":"=a"(v):"a"(1),"D"(&Lock->Lock));
		#endif
	}
	
	#if LOCK_DISABLE_INTS
	Lock->IF = IF;
	#endif
}
/**
 * \brief Release a short lock
 * \param Lock	Lock pointer
 */
static inline void SHORTREL(struct sShortSpinlock *Lock) {
	#if STACKED_LOCKS
	if( Lock->Depth ) {
		Lock->Depth --;
		return ;
	}
	#endif
	
	#if LOCK_DISABLE_INTS
	// Lock->IF can change anytime once Lock->Lock is zeroed
	if(Lock->IF) {
		Lock->Lock = 0;
		__ASM__ ("sti");
	}
	else {
		Lock->Lock = 0;
	}
	#else
	Lock->Lock = 0;
	#endif
}

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
