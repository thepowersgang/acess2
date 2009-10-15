/*
 * Acess2
 * - Virtual Memory Manager (Header)
 */
#ifndef _MM_VIRT_H
#define _MM_VIRT_H

// === FUNCTIONS ===
extern void	MM_SetCR3(Uint32 CR3);
extern tPAddr	MM_Allocate(Uint VAddr);
extern void	MM_Deallocate(Uint VAddr);
extern int	MM_Map(Uint VAddr, tPAddr PAddr);
extern Uint	MM_Clone();
extern Uint	MM_NewKStack();
extern Uint	MM_NewWorkerStack();

#endif
