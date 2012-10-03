/*
 * Acess2 Kernel x86_64
 * - By John Hodge (thePowersGang)
 *
 * include/init.h
 * - Arch-internal init functions
 */
#ifndef _ARCH__INIT_H_
#define _ARCH__INIT_H_

#include <pmemmap.h>

extern void	MM_InitPhys(int NPMemRanges, tPMemMapEnt *PMemRanges);

#endif

