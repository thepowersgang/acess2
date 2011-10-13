/*
 * Acess2 ARMv7 Port
 * - By John Hodge (thePowersGang)
 *
 * options.h
 * - C/ASM Shared constants
 */
#ifndef _ARMV7_OPTIONS_H_
#define _ARMV7_OPTIONS_H_

#define KERNEL_BASE	0x80000000

//#define PCI_PADDR	0x60000000	// Realview (Non-PB)
#define UART0_PADDR	0x10009000	// Realview

#define MM_KSTACK_SIZE	0x2000	// 2 Pages

#endif

