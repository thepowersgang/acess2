/**
 * \file archdoc.h
 * \brief Documentation Definitions for the Acess 2 Architecture Interface
 * \author John Hodge (thePowersGang)
 * 
 * Acess 2 allows different architecture builds to be made off almost
 * the same source tree. The difference between the trees is the inclusion
 * of a different directory from the "Kernel/arch/" directory that
 * contains the architecture specific intialisation and management code.
 * Each achitecture tree must provide all the definitions from this
 * document in some form or another (usually in the most efficient way
 * avaliable)
 * The values and types used in this documentation are a guide only and
 * will most probably be incorrect for most architectures.
 */
#ifndef _ARCHDOC_H_
#define _ARCHDOC_H_

/**
 * \brief Maximum number of CPUs supported by this architecture driver
 *        (in the current build)
 */
#define	MAX_CPUS	1
/**
 * \brief Number of bits in a machine word (Uint)
 */
#define	BITS	32
/**
 * \brief Number of valid bits in a \a tPAddr
 */
#define	PHYS_BITS	32

/**
 * \name Syncronisation Primitives
 * \{
 */
/**
 * \brief Spinlock type
 */
typedef volatile int	tSpinlock;
/**
 * \brief Acquire a spinlock
 */
#define LOCK(lockptr)	do{while(*(tSpinlock*)lockptr)Threads_Yield();*(tSpinlock*)lockptr=1;}while(0)
/**
 * \brief Release a held spinlock
 */
#define RELEASE(lockptr)	do{*(tSpinlock*)lockptr=0;}while(0)
/**
 * \}
 */
//#define	HALT()	__asm__ __volatile__ ("hlt")


/**
 * \name Atomic Types
 * \{
 */
typedef unsigned int	Uint;	//!< Unsigned machine native integer
typedef unsigned char	Uint8;	//!< Unsigned 8-bit integer
typedef unsigned short	Uint16;	//!< Unsigned 16-bit integer
typedef unsigned long	Uint32;	//!< Unsigned 32-bit integer
typedef unsigned long long	Uint64;	//!< Unsigned 64-bit integer
typedef signed int		Sint;	//!< Signed Machine Native integer
typedef signed char		Sint8;	//!< Signed 8-bit integer
typedef signed short	Sint16;	//!< Signed 16-bit integer
typedef signed long		Sint32;	//!< Signed 32-bit integer
typedef signed long long	Sint64;	//!< Signed 16-bit integer
/**
 * \}
 */

typedef Uint	size_t;	//!< Counter/Byte count type
typedef Uint32	tVAddr;	//!< Virtual address type
typedef Uint32	tPAddr;	//!< Physical Address type

/**
 * \brief Register state passed to the syscall handler
 * 
 * The \a tSyscallRegs structure allows the system call handler to read
 * the user state to get the arguments for the call. It also allows the
 * handler to alter specific parts of the user state to reflect the
 * result of the call.
 * \note The fields shown here are need only be present in the actual
 *       structure, in any order. The implementation may also add more
 *       fields for padding purposes.
 */
typedef struct {
	Uint	Arg4;	//!< Fourth argument
	Uint	Arg3;	//!< Third argument
	Uint	Arg2;	//!< Second argument
	union {
		Uint	Arg1;	//!< First arugment
		Uint	RetHi;	//!< High part of the return
	};
	union {
		Uint	Num;	//!< Call Number
		Uint	Return;	//!< Low return value
	};
	
	Uint	StackPointer;	//!< User stack pointer
}	tSyscallRegs;

/**
 * \brief Opaque structure definining the MMU state for a task
 */
typedef struct sMemoryState	tMemoryState;

/**
 * \brief Opque structure defining the CPU state for a task
 */
typedef struct sTaskState	tTaskState;


/**
 * \name Memory Management
 * \{
 */
/**
 * \}
 */


#endif
