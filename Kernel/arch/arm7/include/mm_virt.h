/*
 * Acess2
 * ARM7 Virtual Memory Manager Header
 */
#ifndef _MM_VIRT_H_
#define _MM_VIRT_H_

#define MM_USER_MIN	0x00001000
#define USER_LIB_MAX	0x7F800000
#define MM_PPD_HANDLES	0x7F800000
#define MM_TABLE0USER	0x7F900000	// 2 GiB - 16 KiB
#define MM_TABLE1USER	0x7FC00000	// 2 GiB - 4 MiB

// Page Blocks are 12-bits wide (12 address bits used)
// Hence, the table is 16KiB large (and must be so aligned)
// and each block addresses 1MiB of data

// First level table is aligned to 16KiB (restriction of TTBR reg)
// - VMSAv6 uses two TTBR regs, determined by bit 31

#define KERNEL_BASE	0x80000000	// 2GiB

#define MM_KHEAP_BASE	0x80800000	// 8MiB of kernel code
#define MM_KHEAP_MAX	0xC0000000	// ~1GiB of kernel heap

#define MM_MODULE_MIN	0xC0000000	// - 0xD0000000
#define MM_MODULE_MAX	0xD0000000

// PMM Data, giving it 256MiB is overkill, but it's unused atm
#define MM_MAXPHYSPAGE	(1024*1024)
// 2^(32-12) max pages
// 8.125 bytes per page (for bitmap allocation)
// = 8.125 MiB
#define MM_PMM_BASE	0xE0000000
#define MM_PMM_END	0xF0000000

#define MM_HWMAP_BASE	0xF0000000	// Ent 0xF00
#define MM_HWMAP_END	0xFE000000
#define MM_TMPMAP_BASE	0xFE000000
#define MM_TMPMAP_END	0xFF000000

#define MM_KERNEL_VFS	0xFF000000	// 
#define MM_TABLE1KERN	0xFF800000	// - 0x???????? 4MiB
#define MM_TABLE0KERN	0xFFC00000	// - 0xFFE04000 16KiB

#endif
