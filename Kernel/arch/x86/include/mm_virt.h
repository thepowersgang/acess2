/*
 * Acess2
 * - Virtual Memory Manager (Header)
 */
#ifndef _MM_VIRT_H
#define _MM_VIRT_H


// - Memory Layout
#define	MM_USER_MIN	0x00200000
#define	USER_STACK_SZ	0x00010000
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
#define MM_MODULE_MAX	0xF0000000	// 512 MiB

// === FUNCTIONS ===
extern void	MM_FinishVirtualInit();
extern void	MM_SetCR3(Uint CR3);
extern tPAddr	MM_Allocate(tVAddr VAddr);
extern void	MM_Deallocate(tVAddr VAddr);
extern int	MM_Map(tVAddr VAddr, tPAddr PAddr);
extern tPAddr	MM_Clone();
extern tVAddr	MM_NewKStack();
extern tVAddr	MM_NewWorkerStack();

#endif
