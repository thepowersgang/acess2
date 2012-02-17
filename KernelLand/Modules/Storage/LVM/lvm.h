/*
 * Acess2 Logical Volume Manager
 * - By John Hodge (thePowersGang)
 *
 * lvm.h
 * - LVM Core definitions
 */
#ifndef _LVM_LVM_H_
#define _LVM_LVM_H_

#include <acess.h>

// === TYPES ===
typedef struct sLVM_Vol	tLVM_Vol;

// === FUNCTIONS ===
extern size_t	LVM_int_ReadVolume(tLVM_Vol *Volume, Uint64 BlockNum, size_t BlockCount, void *Dest);
extern size_t	LVM_int_WriteVolume(tLVM_Vol *Volume, Uint64 BlockNum, size_t BlockCount, const void *Src);

// --- Subvolume Management ---
extern void	LVM_int_SetSubvolume_Anon(tLVM_Vol *Volume, int Index, Uint64 FirstBlock, Uint64 LastBlock);

#endif

