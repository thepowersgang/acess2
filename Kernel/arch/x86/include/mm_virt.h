/*
 * Acess2
 * - Virtual Memory Manager (Header)
 */
#ifndef _MM_VIRT_H
#define _MM_VIRT_H

// NOTES:
// - 1PD is 0x400000

// - Memory Layout
#define	MM_USER_MIN	0x00200000
#define	USER_STACK_SZ	0x00020000	// 128 KiB
#define	USER_STACK_TOP	0x00800000
#define USER_LIB_MAX	0xBC000000
#define	MM_USER_MAX	0xBC000000	// Top load address for user libraries
#define	MM_PPD_MIN	0xBC000000	// Per-Process Data base
#define	MM_PPD_HANDLES	0xBC000000	// - VFS Handles (Practically unlimited)
#define	MM_PPD_MMAP	0xBD000000	// - MMap Entries (24b each = 0x2AAAA max)
#define	MM_PPD_UNALLOC	0xBE000000	//
#define MM_PPD_CFG	0xBFFFF000	// - Per-process config entries 
#define	MM_PPD_MAX	0xC0000000	// 

#define	MM_KHEAP_BASE	0xC0400000	// C+4MiB
#define	MM_KHEAP_MAX	0xCF000000	//
#define MM_KERNEL_VFS	0xCF000000	// 
#define MM_KUSER_CODE	0xCFFF0000	// 16 Pages
#define	MM_MODULE_MIN	0xD0000000	// Lowest Module Address
#define MM_MODULE_MAX	0xE0000000	// 128 MiB

// Page Info (Which VFS node owns each physical page)
// 2^32/2^12*16
// = 2^24 = 16 MiB = 0x4000000
// 256 items per page
#define MM_PAGENODE_BASE	0xE0000000

// Needs (2^32/2^12*4 bytes)
// - 2^22 bytes max = 4 MiB = 0x1000000
// 1024 items per page
#define	MM_REFCOUNT_BASE	0xE4000000

#define MM_KERNEL_STACKS	0xF0000000
#define	MM_KERNEL_STACK_SIZE	0x00008000
#define MM_KERNEL_STACKS_END	0xFC000000

// === FUNCTIONS ===
extern void	MM_FinishVirtualInit(void);
extern void	MM_SetCR3(Uint CR3);
extern tPAddr	MM_Clone(int bCloneUser);
extern tVAddr	MM_NewKStack(void);
extern tVAddr	MM_NewWorkerStack(Uint *InitialStack, size_t StackSize);

#endif
