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
typedef struct sLVM_Format tLVM_Format;

// === STRUCTURES ===
struct sLVM_Format
{
	tLVM_Format	*Next;
	const char	*Name;
	 int	(*CountSubvolumes)(tLVM_Vol *Volume, void *FirstBlockData);
	void	(*PopulateSubvolumes)(tLVM_Vol *Volume, void *FirstBlockData);
};

// === FUNCTIONS ===
extern size_t	LVM_int_ReadVolume(tLVM_Vol *Volume, Uint64 BlockNum, size_t BlockCount, void *Dest);
extern size_t	LVM_int_WriteVolume(tLVM_Vol *Volume, Uint64 BlockNum, size_t BlockCount, const void *Src);

// --- Subvolume Management ---
extern void	LVM_int_SetSubvolume_Anon(tLVM_Vol *Volume, int Index, Uint64 FirstBlock, Uint64 LastBlock);

// --- Global Fromats ---
extern tLVM_Format	gLVM_MBRType;

#endif

