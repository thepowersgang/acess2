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
	// Shamelessly copied from linux (/arch/arm/include/asm/spinlock.h) until I can fix stuff
	Uint	tmp;
	__asm__ __volatile__ (
	"1:	ldrex	%0, [%1]\n"
	"	teq	%0, #0\n"
	"	strexeq %0, %2, [%1]\n"	// Magic? TODO: Look up
	"	teqeq	%0, #0\n"
	"	bne	1b"
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

