/*
 * AcessOS Microkernel Version
 * mm_phys.h
 */
#ifndef _MM_PHYS_H
#define _MM_PHYS_H

// === FUNCTIONS ===
extern tPAddr	MM_AllocPhys();
extern tPAddr	MM_AllocPhysRange(int Pages, int MaxBits);
extern void	MM_RefPhys(tPAddr Addr);
extern void	MM_DerefPhys(tPAddr Addr);
extern int	MM_GetRefCount(tPAddr Addr);

#endif
