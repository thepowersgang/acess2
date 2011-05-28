/*
 * Acess2
 * ARM7 Virtual Memory Manager Header
 */
#ifndef _MM_VIRT_H_
#define _MM_VIRT_H_

#define KERNEL_BASE	0x80000000	// 2GiB

#define MM_USER_MIN	0x00001000
#define USER_LIB_MAX	0x7F800000
#define MM_PPD_VFS	0x7F800000

// Page Blocks are 12-bits wide (12 address bits used)
// Hence, the table is 16KiB large (and must be so aligned)
// and each block addresses 1MiB of data

#define MM_KHEAP_BASE	0x80800000	// 8MiB of kernel code
#define MM_KHEAP_MAX	0xC0000000	// 1GiB of kernel heap

#define MM_MODULE_MIN	0xC0000000
#define MM_MODULE_MAX	0xD0000000

#define MM_KERNEL_VFS	0xFF800000	// 
#define MM_FRACTAL	0xFFE00000	// 2nd last block

#endif
