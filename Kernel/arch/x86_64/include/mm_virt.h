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

#define PAGE_SIZE	0x1000

// === Memory Location Definitions ===
/*
 * Userland - Lower Half
 * Kernel land - Upper Half
 * 
 *    START ADDRESS          END ADDRESS       BITS   SIZE      NAME
 * 0x00000000 00000000 - 0x00007FFF FFFFFFFF	47	128 TiB	User Space
 * 0x00008000 00000000 - 0xFFFF7FFF FFFFFFFF	--- SIGN EXTENSION NULL ZONE
 * 0xFFFF8000 00000000 - 0xFFFFFFFF FFFFFFFF	47	128	TiB	Kernel Range
 *       8000 00000000 -       9000 00000000	42	16	TiB	Kernel Heap
 *       9000 00000000 -       9800 00000000	43	8	TiB	Module Space
 *       9800 00000000 -       9A00 00000000	41	2	TiB	Kernel VFS
 *       ---- GAP ----                      		6	TiB
 *       A000 00000000 -       B000 00000000	44	16	TiB	Kernel Stacks
 *       C000 00000000 -       D000 00000000	44	16	TiB	Hardware Mappings
 *       D000 00000000 -       D080 00000000	39	512	GiB	Per-Process Data
 *       D080 00000000 -       D100 00000000	39	512	GiB	Kernel Supplied User Code
 *       ---- GAP ----                      		15	TiB
 *       E000 00000000 -       E800 00000000	43	8	TiB	Physical Page Nodes (2**40 pages * 8 bytes)
 *       E800 00000000 -       EC00 00000000	42	4	TiB	Physical Page Reference Counts (2**40 pg * 4 bytes)
 *       EC00 00000000 -       EC80 00000000	39	512	GiB	Physical Page Bitmap (1 page per bit)
 *       EC80 00000000 -       ED00 00000000	39	512	GiB	Physical Page DblAlloc Bitmap (1 page per bit)
 *       ED00 00000000 -       ED00 80000000	31	2	GiB	Physical Page Super Bitmap (64 pages per bit)
 *       ---- GAP ----                      		9	TiB
 *       FE00 00000000 -       FE80 00000000	39	512	GiB	Fractal Mapping (PML4 508)
 *       FE80 00000000 -       FF00 00000000	39	512	GiB	Temp Fractal Mapping
 *       FF00 00000000 -       FF80 00000000	39	512	GiB	Temporary page mappings
 *       FF80 00000000 -       FF80 80000000	31	2	GiB	Local APIC
 *       ---- GAP ----                      	  	506	GiB
 *       FFFF 00000000 -       FFFF 80000000	31	2	GiB	User Code
 *       FFFF 80000000 -       FFFF FFFFFFFF	31	2	GiB	Kernel code / data
 */

#define	MM_USER_MIN 	0x00000000##00010000
#define USER_LIB_MAX	0x00007000##00000000
#define USER_STACK_SZ	0x00000000##00020000	// 64 KiB
#define USER_STACK_TOP	0x00007FFF##FFFFF000
#define	MM_USER_MAX 	0x00007FFF##FFFFF000
#define	MM_KERNEL_RANGE	0xFFFF8000##00000000
#define MM_KHEAP_BASE	(MM_KERNEL_RANGE|(0x8000##00000000))
#define MM_KHEAP_MAX	(MM_KERNEL_RANGE|(0x9000##00000000))
#define MM_MODULE_MIN	(MM_KERNEL_RANGE|(0x9000##00000000))
#define MM_MODULE_MAX	(MM_KERNEL_RANGE|(0x9800##00000000))
#define MM_KERNEL_VFS	(MM_KERNEL_RANGE|(0x9800##00000000))
#define MM_KSTACK_BASE	(MM_KERNEL_RANGE|(0xA000##00000000))
#define MM_KSTACK_TOP	(MM_KERNEL_RANGE|(0xB000##00000000))

#define MM_HWMAP_BASE	(MM_KERNEL_RANGE|(0xC000##00000000))
#define MM_HWMAP_TOP	(MM_KERNEL_RANGE|(0xD000##00000000))
#define MM_PPD_BASE 	(MM_KERNEL_RANGE|(0xD000##00000000))
#define MM_PPD_CFG  	MM_PPD_BASE
#define MM_PPD_HANDLES 	(MM_KERNEL_RANGE|(0xD008##00000000))
#define MM_USER_CODE	(MM_KERNEL_RANGE|(0xD080##00000000))

#define MM_PAGE_NODES	(MM_KERNEL_RANGE|(0xE000##00000000))
#define MM_PAGE_COUNTS	(MM_KERNEL_RANGE|(0xE800##00000000))
#define MM_PAGE_BITMAP	(MM_KERNEL_RANGE|(0xEC00##00000000))
#define MM_PAGE_DBLBMP	(MM_KERNEL_RANGE|(0xEC00##00000000))
#define MM_PAGE_SUPBMP	(MM_KERNEL_RANGE|(0xED00##00000000))

#define MM_FRACTAL_BASE	(MM_KERNEL_RANGE|(0xFE00##00000000))
#define MM_TMPFRAC_BASE	(MM_KERNEL_RANGE|(0xFE80##00000000))
#define MM_TMPMAP_BASE	(MM_KERNEL_RANGE|(0xFF00##00000000))
#define MM_TMPMAP_END	(MM_KERNEL_RANGE|(0xFF80##00000000))
#define MM_LOCALAPIC	(MM_KERNEL_RANGE|(0xFF80##00000000))
#define MM_KERNEL_CODE	(MM_KERNEL_RANGE|(0xFFFF##80000000))


// === FUNCTIONS ===
void	MM_FinishVirtualInit(void);
tVAddr	MM_NewKStack(void);
tVAddr	MM_Clone(void);
tVAddr	MM_NewWorkerStack(void *StackData, size_t StackSize);

#endif
