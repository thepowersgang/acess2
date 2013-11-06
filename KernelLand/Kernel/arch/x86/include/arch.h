/*
 * Acess2
 * - x86 Architecture
 * arch/x86/include/arch.h
 */
#ifndef _ARCH_H_
#define _ARCH_H_

// - Base Defintions
#define	KERNEL_BASE	0xC0000000
#define BITS	32
#define PAGE_SIZE	0x1000

#define INVLPTR	((void*)-1)

// Allow nested spinlocks?
#define LOCK_DISABLE_INTS	1

// - Processor/Machine Specific Features
#if ARCH != x86 && ARCH != x86_smp
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
	void	*LockedBy;
};

// === MACROS ===
/**
 * \brief Halt the CPU (shorter version of yield)
 */
#if 1
#define	HALT()	do { \
	Uint32	flags; \
	__asm__ __volatile__ ("pushf;pop %0" : "=a"(flags)); \
	if( !(flags & 0x200) )	Panic("HALT called with interrupts disabled"); \
	__asm__ __volatile__ ("hlt"); \
} while(0)
#else
#define	HALT()	__asm__ __volatile__ ("hlt")
#endif
/**
 * \brief Fire a magic breakpoint (bochs)
 */
#define	MAGIC_BREAK()	__asm__ __volatile__ ("xchg %bx, %bx")
// TODO: SMP halt request too
#define HALT_CPU()	for(;;) { __asm__ __volatile__ ("cli; hlt"); }

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
typedef char	BOOL;

typedef Uint32	tPAddr;
typedef Uint32	tVAddr;

typedef struct {
	Uint32	gs, fs, es, ds;
	Uint32	edi, esi, ebp, kesp;
	Uint32	ebx, edx, ecx, eax;
	Uint32	int_num, err_code;
	Uint32	eip, cs;
	Uint32	eflags, esp, ss;
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

// === FUNCTIONS ===
extern void	Debug_PutCharDebug(char ch);
extern void	Debug_PutStringDebug(const char *String);

extern int	IS_LOCKED(struct sShortSpinlock *Lock);
extern int	CPU_HAS_LOCK(struct sShortSpinlock *Lock);
extern void	SHORTLOCK(struct sShortSpinlock *Lock);
extern void	SHORTREL(struct sShortSpinlock *Lock);

#endif	// !defined(_ARCH_H_)
