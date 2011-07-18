/*
 * Acess2
 * - Virtual Memory Manager (Header)
 */
#ifndef _MM_VIRT_H
#define _MM_VIRT_H


// - Memory Layout
#define	MM_USER_MIN	0x00200000
#define	USER_STACK_SZ	0x00020000	// 128 KiB
#define	USER_STACK_TOP	0x00800000
#define USER_LIB_MAX	0xBC000000
#define	MM_USER_MAX	0xBC000000	// Top load address for user libraries
#define	MM_PPD_MIN	0xBC000000	// Per-Process Data
#define	MM_PPD_VFS	0xBC000000	// 
#define MM_PPD_CFG	0xBFFFF000	// 
#define	MM_PPD_MAX	0xC0000000	// 

#define	MM_KHEAP_BASE	0xC0400000	// C+4MiB
#define	MM_KHEAP_MAX	0xCF000000	//
#define MM_KERNEL_VFS	0xCF000000	// 
#define MM_KUSER_CODE	0xCFFF0000	// 16 Pages
#define	MM_MODULE_MIN	0xD0000000	// Lowest Module Address
#define MM_MODULE_MAX	0xE0000000	// 128 MiB
// Needs (2^36/2^12*8 bytes)
// - 2^27 bytes max = 128 MiB = 0x10000000
// 2^12/2^3 items per page
// - 2^9 = 512
#define	MM_PAGEINFO_BASE	0xE0000000

// === FUNCTIONS ===
extern void	MM_FinishVirtualInit(void);
extern void	MM_SetCR3(Uint CR3);
extern tPAddr	MM_Clone(void);
extern tVAddr	MM_NewKStack(void);
extern tVAddr	MM_NewWorkerStack(void);

#endif
