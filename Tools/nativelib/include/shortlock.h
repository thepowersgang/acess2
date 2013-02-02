/*
 * Acess2 native library
 * - By John Hodge (thePowersGang)
 *
 * shortlock.h
 * - Short locks :)
 */
#ifndef _SHORTLOCK_H_
#define _SHORTLOCK_H_

typedef void	*tShortSpinlock;

extern void	SHORTLOCK(tShortSpinlock *Lock);
extern void	SHORTREL(tShortSpinlock *m);
extern int	CPU_HAS_LOCK(tShortSpinlock *m);

#endif
