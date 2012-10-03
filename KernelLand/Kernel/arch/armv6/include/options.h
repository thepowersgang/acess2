/*
 * Acess2 ARMv6 Port
 * - By John Hodge (thePowersGang)
 *
 * options.h
 * - C/ASM Shared constants
 */
#ifndef _ARMV7_OPTIONS_H_
#define _ARMV7_OPTIONS_H_

#define KERNEL_BASE	0x80000000

#if PLATFORM_is_raspberrypi
# define UART0_PADDR	0x7E215040	// Realview
#else
# error Unknown platform
#endif

#define MM_KSTACK_SIZE	0x2000	// 2 Pages

#endif

