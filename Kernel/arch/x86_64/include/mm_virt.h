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
 * 0xFFFF8000 00000000 - 0xFFFFFFFF FFFFFFFF	2**47	Kernel Range
 *       8000 00000000 -       8000 7FFFFFFF	2 GiB	Identity Map
 *       8000 80000000 -       8001 00000000	2 GiB	Kernel Heap
 *       9000 00000000 -       9800 00000000	cbf	Module Space
 *       9800 00000000 -       9900 00000000	cbf	Per-Process Data
 *       9900 00000000 -       9A00 00000000	cbf	Kernel VFS
 */

#define	MM_USER_MIN	0x00008FFF##FFFFF000
//#define	KERNEL_BASE	0xFFF8000##00000000
#define MM_KHEAP_BASE	(KERNEL_BASE|(0x0000##80000000))
#define MM_KHEAP_MAX	(KERNEL_BASE|(0x0001##00000000))
#define MM_MODULE_MIN	(KERNEL_BASE|(0x1000##00000000))
#define MM_MODULE_MAX	(KERNEL_BASE|(0x1800##00000000))
#define MM_PPD_BASE	(KERNEL_BASE|(0x1800##00000000))
#define MM_PPD_VFS	(KERNEL_BASE|(0x1880##00000000))
#define MM_KERNEL_VFS	(KERNEL_BASE|(0x1900##00000000))

#endif
