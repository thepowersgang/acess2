/*
 * AcessOS Microkernel Version
 * mm_phys.h
 */
#ifndef _MM_PHYS_H
#define _MM_PHYS_H

// === FUNCTIONS ===
extern void	MM_SetCR3(Uint32 CR3);
extern tPAddr	MM_Allocate(Uint VAddr);
extern void	MM_Deallocate(Uint VAddr);
extern int	MM_Map(Uint VAddr, tPAddr PAddr);
extern Uint	MM_Clone();
extern Uint	MM_NewKStack();

#endif
