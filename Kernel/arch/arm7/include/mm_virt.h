/*
 * Acess2
 * ARM7 Virtual Memory Manager Header
 */
#ifndef _MM_VIRT_H_
#define _MM_VIRT_H_

#define KERNEL_BASE	0x80000000	// 2GiB

// Page Blocks are 12-bits wide (12 address bits used)
// Hence, the table is 16KiB large (and must be so aligned)
// and each block addresses 1MiB of data

#define MM_KHEAP_BASE	0x80800000	// 8MiB of kernel code
#define MM_KHEAP_MAX	0xC0000000	// 1GiB of kernel heap

#define MM_FRACTAL	0xFFE00000	// 2nd last block

#endif
