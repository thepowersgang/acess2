/*
 * Acess2 Logical Volume Manager
 * - By John Hodge (thePowersGang)
 *
 * volumes.c
 * - Volume management
 */
#include "lvm_int.h"

// === PROTOTYPES ===
 int	LVM_int_VFSReadEmul(void *Arg, Uint64 BlockStart, size_t BlockCount, void *Dest);
 int	LVM_int_VFSWriteEmul(void *Arg, Uint64 BlockStart, size_t BlockCount, const void *Source);

// === CODE ===
// --------------------------------------------------------------------
// Managment / Initialisation
// --------------------------------------------------------------------
int LVM_AddVolumeVFS(const char *Name, int FD)
{
	return LVM_AddVolume(Name, (void*)(Uint)FD, LVM_int_VFSReadEmul, LVM_int_VFSWriteEmul);
}

int LVM_AddVolume(const char *Name, void *Ptr, tLVM_ReadFcn Read, tLVM_WriteFcn Write)
{
	tLVM_Vol	dummy_vol;
//	tLVM_Vol	*real_vol;

	dummy_vol.Ptr = Ptr;
	dummy_vol.Read = Read;
	dummy_vol.Write = Write;
	
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
	return Volume->Read(Volume->Ptr, BlockNum, BlockCount, Dest);
}

size_t LVM_int_WriteVolume(tLVM_Vol *Volume, Uint64 BlockNum, size_t BlockCount, const void *Src)
{
	return Volume->Write(Volume->Ptr, BlockNum, BlockCount, Src);	
}

int LVM_int_VFSReadEmul(void *Arg, Uint64 BlockStart, size_t BlockCount, void *Dest)
{
	size_t	blocksize;
	size_t	rv;

	blocksize = 512;	// TODO: Don't assume	

	rv = VFS_ReadAt( (int)(Uint)Arg, BlockStart * blocksize, BlockCount * blocksize, Dest );
	rv /= blocksize;
	return rv;
}

int LVM_int_VFSWriteEmul(void *Arg, Uint64 BlockStart, size_t BlockCount, const void *Source)
{
	size_t	blocksize;
	size_t	rv;

	blocksize = 512;	// TODO: Don't assume	

	rv = VFS_WriteAt( (int)(Uint)Arg, BlockStart * blocksize, BlockCount * blocksize, Source );
	rv /= blocksize;
	return rv;
}

