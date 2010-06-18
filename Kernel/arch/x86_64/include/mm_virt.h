/*
 * Acess2 x86_64 Architecture Code
 *
 * This file is published under the terms of the Acess Licence.
 * See the file COPYING for more details
 *
 * vmem.h - Virtual Memory Functions & Definitions
 */
#ifndef _VMEM_H_
#define _VMEM_H_

#include <arch.h>

// === Memory Location Definitions ===
/*
 * Userland - Lower Half
 * Kernel land - Upper Half
 * 
 *    START ADDRESS          END ADDRESS       BITS   SIZE      NAME
 * 0x00000000 00000000 - 0x00007FFF FFFFFFFF	47	128 TiB	User Space
 * 0x00008000 00000000 - 0xFFFF7FFF FFFFFFFF	--- SIGN EXTENSION NULL ZONE
 * 0xFFFF8000 00000000 - 0xFFFFFFFF FFFFFFFF	47	128 TiB	Kernel Range
 *       8000 00000000 -       9000 00000000	44	16  TiB	Kernel Heap
 *       9000 00000000 -       9800 00000000	43	8   TiB	Module Space
 *       9800 00000000 -       9A00 00000000	41	2   TiB	Kernel VFS
 *       A000 00000000 -       B000 00000000	44	16  TiB	Kernel Stacks
 *       C000 00000000 -       D000 00000000	44	16  TiB	Hardware Mappings
 *       D000 00000000 -       D080 00000000	39	512	GiB	Per-Process Data
 *       D080 00000000 -       D100 00000000	39	512	GiB	Kernel Supplied User Code
 *       E000 00000000 -       E400 00000000	42	4	TiB	Physical Page Reference Counts (2**40 = 2**52 bytes)
 *       E400 00000000 -       E480 00000000	39	512	GiB	Physical Page Bitmap (1 page per bit)
 *       E480 00000000 -       E500 00000000	39	512	GiB	Physical Page DblAlloc Bitmap (1 page per bit)
 *       E500 00000000 -       E500 80000000	31	2	GiB	Physical Page Super Bitmap (64 pages per bit)
 *       FD00 00000000 -       FD80 00000000	39	512 GiB	Local APIC
 *       FE00 00000000 -       FE80 00000000	39	512 GiB	Fractal Mapping (PML4 510)
 *       FE80 00000000 -       FF00 00000000	39	512 GiB	Temp Fractal Mapping
 *       FF00 00000000 -       FF80 00000000	39	512 GiB	-- UNUSED --
 *       FF80 00000000 -       FFFF 80000000	~39		GiB	-- UNUSED --
 *       FFFF 80000000 -       FFFF 7FFFFFFF	31	2   GiB	Identity Map
 */

#define	MM_USER_MIN 	0x00000000##00010000
#define USER_LIB_MAX	0x00007000##00000000
#define USER_STACK_SZ	0x00000000##00020000	// 64 KiB
#define USER_STACK_TOP	0x00007FFF##FFFFF000
#define	MM_USER_MAX 	0x00007FFF##FFFFF000
//#define	KERNEL_BASE	0xFFFF8000##00000000
#define MM_KHEAP_BASE	(KERNEL_BASE|(0x8000##80000000))
#define MM_KHEAP_MAX	(KERNEL_BASE|(0x9000##00000000))
#define MM_MODULE_MIN	(KERNEL_BASE|(0x9000##00000000))
#define MM_MODULE_MAX	(KERNEL_BASE|(0x9800##00000000))
#define MM_KERNEL_VFS	(KERNEL_BASE|(0x9800##00000000))
#define MM_KSTACK_BASE	(KERNEL_BASE|(0xA000##00000000))
#define MM_KSTACK_TOP	(KERNEL_BASE|(0xB000##00000000))

#define MM_HWMAP_BASE	(KERNEL_BASE|(0xC000##00000000))
#define MM_HWMAP_TOP	(KERNEL_BASE|(0xD000##00000000))
#define MM_PPD_BASE 	(KERNEL_BASE|(0xD000##00000000))
#define MM_PPD_CFG  	MM_PPD_BASE
#define MM_PPD_VFS  	(KERNEL_BASE|(0xD008##00000000))
#define MM_USER_CODE	(KERNEL_BASE|(0xD080##00000000))

#define MM_PAGE_COUNTS	(KERNEL_BASE|(0xE000##00000000))
#define MM_PAGE_BITMAP	(KERNEL_BASE|(0xE400##00000000))
#define MM_PAGE_DBLBMP	(KERNEL_BASE|(0xE480##00000000))
#define MM_PAGE_SUPBMP	(KERNEL_BASE|(0xE500##00000000))

#define MM_LOCALAPIC	(KERNEL_BASE|(0xFD00##00000000))
#define MM_FRACTAL_BASE	(KERNEL_BASE|(0xFE00##00000000))
#define MM_TMPFRAC_BASE	(KERNEL_BASE|(0xFE80##00000000))


// === FUNCTIONS ===
void	MM_FinishVirtualInit(void);
tVAddr	MM_NewKStack(void);
tVAddr	MM_Clone(void);
tVAddr	MM_NewWorkerStack(void);

#endif
