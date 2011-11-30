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

#if CONFIG_is_realview_pb
# define UART0_PADDR	0x10009000	// Realview
# define GICI_PADDR	0x1e000000
# define GICD_PADDR	0x1e001000
# define PL110_BASE	0x10020000	// Integrator

#endif

#if CONFIG_is_trimslice	// Tegra2
# define UART0_PADDR	0x70006000
# define GICD_PADDR	0x50041000
# define GICI_PADDR	0x60004000	// TODO: Is this actually a GIC-I?
//# define PL110_BASE	0x10020000	// Integrator
#endif

#define MM_KSTACK_SIZE	0x2000	// 2 Pages

#endif

