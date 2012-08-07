/*
 * Acess2 Logical Volume Manager
 * - By John Hodge (thePowersGang)
 *
 * volumes.c
 * - Volume management
 */
#define DEBUG	1
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
	// Assuming 512-byte blocks, not a good idea
//	return LVM_AddVolume(Name, (void*)(Uint)FD, 512, LVM_int_VFSReadEmul, LVM_int_VFSWriteEmul);
	return 0;
}

int LVM_AddVolume(const tLVM_VolType *Type, const char *Name, void *Ptr, size_t BlockSize, size_t BlockCount)
{
	tLVM_Vol	dummy_vol;
	tLVM_Vol	*real_vol;
	tLVM_Format	*fmt;
	void	*first_block;

	if( BlockCount == 0 || BlockSize == 0 ) {
		Log_Error("LVM", "BlockSize(0x%x)/BlockCount(0x%x) invalid in LVM_AddVolume",
			BlockSize, BlockCount);
		return 1;
	}

	dummy_vol.Type = Type;
	dummy_vol.Ptr = Ptr;
	dummy_vol.BlockCount = BlockCount;
	dummy_vol.BlockSize = BlockSize;

	// Read the first block of the volume	
	first_block = malloc(BlockSize);
	if( !first_block ) {
		Log_Error("VLM", "LVM_AddVolume - malloc error on %i bytes", BlockSize);
		return -1;
	}
	Type->Read(Ptr, 0, 1, first_block);
	
	// Determine Format
	// TODO: Determine format
	fmt = &gLVM_MBRType;

	// Type->CountSubvolumes
	dummy_vol.nSubVolumes = fmt->CountSubvolumes(&dummy_vol, first_block);
	
	// Create real volume descriptor
	// TODO: If this needs to be rescanned later, having the subvolume list separate might be an idea
	real_vol = malloc( sizeof(tLVM_Vol) + strlen(Name) + 1 + sizeof(tLVM_SubVolume*) * dummy_vol.nSubVolumes );
	real_vol->Next = NULL;
	real_vol->Type = Type;
	real_vol->Ptr = Ptr;
	real_vol->BlockSize = BlockSize;
	real_vol->BlockCount = BlockCount;
	real_vol->nSubVolumes = dummy_vol.nSubVolumes;
	real_vol->SubVolumes = (void*)( real_vol->Name + strlen(Name) + 1 );
	real_vol->BlockSize = BlockSize;
	strcpy(real_vol->Name, Name);
	memset(real_vol->SubVolumes, 0, sizeof(tLVM_SubVolume*) * real_vol->nSubVolumes);
	// - VFS Nodes
	memset(&real_vol->DirNode, 0, sizeof(tVFS_Node));
	real_vol->DirNode.Type = &gLVM_VolNodeType;
	real_vol->DirNode.ImplPtr = real_vol;
	real_vol->DirNode.Flags = VFS_FFLAG_DIRECTORY;
	real_vol->DirNode.Size = -1;
	memset(&real_vol->VolNode, 0, sizeof(tVFS_Node));
	real_vol->VolNode.Type = &gLVM_VolNodeType;
	real_vol->VolNode.ImplPtr = real_vol;
	real_vol->VolNode.Size = BlockCount * BlockSize;

	// Type->PopulateSubvolumes
	fmt->PopulateSubvolumes(real_vol, first_block);
	free(first_block);

	// Add to volume list
	gpLVM_LastVolume->Next = real_vol;
	gpLVM_LastVolume = real_vol;

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
	sv->Node.Size = BlockCount * Volume->BlockSize;
	
	Log_Log("LVM", "Partition %s/%s - 0x%llx+0x%llx blocks",
		Volume->Name, sv->Name, FirstBlock, BlockCount);
}

// --------------------------------------------------------------------
// IO
// --------------------------------------------------------------------
size_t LVM_int_ReadVolume(tLVM_Vol *Volume, Uint64 BlockNum, size_t BlockCount, void *Dest)
{
	return Volume->Type->Read(Volume->Ptr, BlockNum, BlockCount, Dest);
}

size_t LVM_int_WriteVolume(tLVM_Vol *Volume, Uint64 BlockNum, size_t BlockCount, const void *Src)
{
	return Volume->Type->Write(Volume->Ptr, BlockNum, BlockCount, Src);	
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

