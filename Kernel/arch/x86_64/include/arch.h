/*
 * Acess2 x86-64 Architecure Module
 * - By John Hodge (thePowersGang)
 */
#ifndef _ARCH_H_
#define _ARCH_H_

#define KERNEL_BASE	0xFFFF8000##00000000

// === Core Types ===
typedef signed char	Sint8;
typedef unsigned char	Uint8;
typedef signed short	Sint16;
typedef unsigned short	Uint16;
typedef signed long	Sint32;
typedef unsigned long	Uint32;
typedef signed long long	Sint64;
typedef unsigned long long	Uint64;

typedef Uint64	Uint;
typedef Uint64	tPAddr;
typedef Uint64	tVAddr;

//typedef unsigned int	size_t;
typedef Uint64	size_t;

typedef int	tSpinlock;

#define LOCK(_ptr)
#define RELEASE(_ptr)

#endif

