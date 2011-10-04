/*
 * Acess2
 * ARM7 Architecture Header
 */
#ifndef _ARCH_H_
#define _ARCH_H_

// === CONSTANTS ===
#define INVLPTR	((void*)-1)
#define BITS	32
#define PAGE_SIZE	0x1000
#define KERNEL_BASE	0x80000000	// 2GiB

// === TYPES ===
typedef unsigned int	Uint;
typedef unsigned char	Uint8;
typedef unsigned short	Uint16;
typedef unsigned long	Uint32;
typedef unsigned long long	Uint64;
typedef signed int	Sint;
typedef signed char	Sint8;
typedef signed short	Sint16;
typedef signed long	Sint32;
typedef signed long long	Sint64;

typedef int	size_t;
typedef char	BOOL;

typedef Uint32	tVAddr;
typedef Uint32	tPAddr;

#include "lock.h"

// --- Debug
extern void	Debug_PutCharDebug(char Ch);
extern void	Debug_PutStringDebug(const char *String);

// This should be elsewhere, but CBF
extern void	MM_SetupPhys(void);
extern int	MM_InitialiseVirtual(void);

#define NO_IO_BUS	1

#endif
