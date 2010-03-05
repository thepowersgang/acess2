/*
 * AcessOS Microkernel Version
 * mm_phys.h
 */
#ifndef _MM_PHYS_H
#define _MM_PHYS_H

// === FUNCTIONS ===
extern tPAddr	MM_AllocPhys();
extern tPAddr	MM_AllocPhysRange(int Pages, int MaxBits);
extern void	MM_RefPhys(tPAddr PAddr);
extern void	MM_DerefPhys(tPAddr PAddr);
extern int	MM_GetRefCount(tPAddr Addr);

#endif
