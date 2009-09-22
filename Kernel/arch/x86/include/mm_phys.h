/*
 * AcessOS Microkernel Version
 * mm_phys.h
 */
#ifndef _MM_PHYS_H
#define _MM_PHYS_H

// === FUNCTIONS ===
extern tPAddr	MM_AllocPhys();
extern void	MM_RefPhys(Uint32 Addr);
extern void	MM_DerefPhys(Uint32 Addr);
extern int	MM_GetRefCount(tPAddr Addr);

#endif
