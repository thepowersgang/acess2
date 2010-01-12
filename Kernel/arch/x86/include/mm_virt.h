/*
 * Acess2
 * - Virtual Memory Manager (Header)
 */
#ifndef _MM_VIRT_H
#define _MM_VIRT_H

// === FUNCTIONS ===
extern void	MM_FinishVirtualInit();
extern void	MM_SetCR3(Uint32 CR3);
extern tPAddr	MM_Allocate(tVAddr VAddr);
extern void	MM_Deallocate(tVAddr VAddr);
extern int	MM_Map(tVAddr VAddr, tPAddr PAddr);
extern tPAddr	MM_Clone();
extern tVAddr	MM_NewKStack();
extern tVAddr	MM_NewWorkerStack();

#endif
