/*
 * Acess2
 * ARM7 Architecture
 *
 * lock.h - Hardware level spinlocks
 */
#ifndef _LOCK_H_
#define _LOCK_H_

// === CODE ===
struct sShortSpinlock {
	 int	Lock;
};

// --- Spinlocks ---
static inline int IS_LOCKED(struct sShortSpinlock *Lock)
{
	return !!Lock->Lock;
}

static inline int CPU_HAS_LOCK(struct sShortSpinlock *Lock)
{
	// TODO: Handle multiple CPUs
	return !!Lock->Lock;
}

static inline int SHORTLOCK(struct sShortSpinlock *Lock)
{
	// Coped from linux, yes, but I know what it does now :)
	Uint	tmp;
	__asm__ __volatile__ (
	"1:	ldrex	%0, [%1]\n"	// Exclusive LOAD
	"	teq	%0, #0\n"	// Check if zero
	"	strexeq %0, %2, [%1]\n"	// Set to one if it is zero (releasing lock on the memory)
	"	teqeq	%0, #0\n"	// If the lock was avaliable, check if the write succeeded
	"	bne	1b"	// If the lock was unavaliable, or the write failed, loop
		: "=&r" (tmp)	// Temp
		: "r" (&Lock->Lock), "r" (1)
		: "cc"	// Condition codes clobbered
		);
	return 1;
}

static inline void SHORTREL(struct sShortSpinlock *Lock)
{
	Lock->Lock = 0;
}

#endif

