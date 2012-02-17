/*
 * Acess2 Logical Volume Manager
 * - By John Hodge (thePowersGang)
 *
 * volumes.c
 * - Volume management
 */
#include "lvm_int.h"

// === PROTOTYPES ===

// === CODE ===
// --------------------------------------------------------------------
// Managment / Initialisation
// --------------------------------------------------------------------
int LVM_AddVolume(const char *Name, int FD)
{
	// Make dummy volume descriptor (for the read code)

	// Determine Type

	// Type->CountSubvolumes
	
	// Create real volume descriptor

	// Type->PopulateSubvolumes

	// Add to volume list

	return 0;
}

void LVM_int_SetSubvolume_Anon(tLVM_Vol *Volume, int Index, Uint64 FirstBlock, Uint64 BlockCount)
{
	tLVM_SubVolume	*sv;
	 int	namelen;

	if( Index < 0 || Index >= Volume->nSubVolumes ) {
		Log_Warning("LVM", "SV ID is out of range (0 < %i < %i)",
			Index, Volume->nSubVolumes);
		return ;
	}

	if( Volume->SubVolumes[Index] ) {
		Log_Warning("LVM", "Attempt to set SV %i of %p twice", Index, Volume);
		return ;
	}
	
	namelen = snprintf(NULL, 0, "%i", Index);

	sv = malloc( sizeof(tLVM_SubVolume) + namelen + 1 );
	if(!sv) {
		// Oh, f*ck
		return ;
	}
	Volume->SubVolumes[Index] = sv;	

	sv->Vol = Volume;
	sprintf(sv->Name, "%i", Index);
	sv->FirstBlock = FirstBlock;
	sv->BlockCount = BlockCount;
	memset(&sv->Node, 0, sizeof(tVFS_Node));
	
	sv->Node.ImplPtr = sv;
	sv->Node.Type = &gLVM_SubVolNodeType;
}

// --------------------------------------------------------------------
// IO
// --------------------------------------------------------------------
size_t LVM_int_ReadVolume(tLVM_Vol *Volume, Uint64 BlockNum, size_t BlockCount, void *Dest)
{
	size_t	rv;
	rv = VFS_ReadAt(
		Volume->BackingDescriptor,
		BlockNum * Volume->BlockSize,
		BlockCount * Volume->BlockSize,
		Dest
		);
	return rv / Volume->BlockSize;
}

size_t LVM_int_WriteVolume(tLVM_Vol *Volume, Uint64 BlockNum, size_t BlockCount, const void *Src)
{
	size_t	rv;
	rv = VFS_WriteAt(
		Volume->BackingDescriptor,
		BlockNum * Volume->BlockSize,
		BlockCount * Volume->BlockSize,
		Src
		);
	return rv / Volume->BlockSize;
}

