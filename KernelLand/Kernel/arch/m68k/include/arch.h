/*
 * Acess2 M68000 port
 * - By John Hodge (thePowersGang)
 *
 * arch/m68k/include/arch.h
 * - Architectre config
 */
#ifndef _M68K_ARCH_H_
#define _M68K_ARCH_H_

#define INVLPTR	((void*)-1)
#define BITS	32

typedef unsigned long long	Uint64;
typedef unsigned long	Uint32;
typedef unsigned short	Uint16;
typedef unsigned char	Uint8;

typedef signed long long	Sint64;
typedef signed long	Sint32;
typedef signed short	Sint16;
typedef signed char	Sint8;

typedef unsigned int	Uint;
typedef unsigned int	size_t;

typedef char	BOOL;

typedef Uint32	tVAddr;
typedef Uint32	tPAddr;

struct sShortSpinlock
{
	int v;
};

// Non preemptive and no SMP, no need for these
#define SHORTLOCK(lock)	do{}while(0)
#define SHORTREL(lock)	do{}while(0)
#define IS_LOCKED(lock)	(0)
#define CPU_HAS_LOCK(lock)	(0)

#define Debug_PutCharDebug(ch)	do{}while(0)
#define Debug_PutStringDebug(ch)	do{}while(0)

#define HALT()	do{}while(0)

#define USER_MAX	0

#define	PAGE_SIZE	0x1000

#endif

