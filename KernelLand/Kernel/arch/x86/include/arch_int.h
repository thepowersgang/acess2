/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * x86 Arch - Internal Definitions
 * - arch/x86/include/arch_int.h
 */
#ifndef _ARCH_INT_H_
#define _ARCH_INT_H_

/**
 * \brief Spinlock primative atomic set-if-zero loop
 */
extern void	__AtomicTestSetLoop(Uint *Ptr, Uint Value);

/**
 * \brief Clear and free an address space
 */
extern void	MM_ClearSpace(Uint32 CR3);

/**
 * \brief Print a backtrace using the supplied IP/BP
 */
void	Error_Backtrace(Uint EIP, Uint EBP);

#endif

