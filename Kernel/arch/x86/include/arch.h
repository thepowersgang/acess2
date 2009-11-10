/*
 * Acess2
 * - x86 Architecture
 * arch/i386/include/arch.h
 */
#ifndef _ARCH_H_
#define _ARCH_H_

// - Memory Layout
#define	MM_USER_MIN	0x00200000
#define	USER_STACK_SZ	0x00010000
#define	USER_STACK_TOP	0x00800000
#define	MM_USER_MAX	0xBC000000
#define	MM_PPD_MIN	0xBC000000	// Per-Process Data
#define	MM_PPD_VFS	0xBC000000	// 
#define MM_PPD_CFG	0xBFFFF000	// 
#define	MM_PPD_MAX	0xB0000000
#define	KERNEL_BASE	0xC0000000
#define	MM_KHEAP_BASE	0xC0400000	// C+4MiB
#define	MM_KHEAP_MAX	0xCF000000	//
#define MM_KERNEL_VFS	0xCF000000	// 
#define MM_KUSER_CODE	0xCFFF0000	// 16 Pages
#define	MM_MODULE_MIN	0xD0000000	// Lowest Module Address
#define MM_MODULE_MAX	0xF0000000	// 512 MiB

#define BITS	32

// - Processor/Machine Specific Features
#if ARCH == i386
// Uses no advanced features
# define	USE_MP	0
# define	USE_PAE	0
#elif ARCH == i586
// All Enabled
# define	USE_MP	1
# define	USE_PAE	1
#else
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

// === MACROS ===
#define LOCK(lockptr)	do {int v=1;\
	while(v)__asm__ __volatile__("lock xchgl %%eax, (%%edi)":"=a"(v):"a"(1),"D"(lockptr));}while(0)
#define	RELEASE(lockptr)	__asm__ __volatile__("lock andl $0, (%%edi)"::"D"(lockptr));
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

#if USE_PAE
typedef Uint64	tPAddr;
#else
typedef Uint32	tPAddr;
#endif
typedef Uint32	tVAddr;

typedef void (*tThreadFunction)(void*);

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
	Uint16	LimitLow;
	Uint16	BaseLow;
	Uint8	BaseMid;
	Uint8	Access;
	struct {
		unsigned LimitHi:	4;
		unsigned Flags:		4;
	} __attribute__ ((packed));
	Uint8	BaseHi;
} __attribute__ ((packed)) tGDT;

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

// --- Interface Flags & Macros
#define CLONE_VM	0x10

#endif	// !defined(_ARCH_H_)
