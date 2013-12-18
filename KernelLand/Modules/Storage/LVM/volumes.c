/*
 * Acess2 Logical Volume Manager
 * - By John Hodge (thePowersGang)
 *
 * volumes.c
 * - Volume management
 */
#define DEBUG	0
#include "lvm_int.h"
#define USE_IOCACHE	1

// === PROTOTYPES ===
 int	LVM_int_CacheWriteback(void *ID, Uint64 Sector, const void *Buffer);
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

void *LVM_AddVolume(const tLVM_VolType *Type, const char *Name, void *Ptr, size_t BlockSize, size_t BlockCount)
{
	tLVM_Vol	dummy_vol;
	tLVM_Format	*fmt;

	if( BlockCount == 0 || BlockSize == 0 ) {
		Log_Error("LVM", "BlockSize(0x%x)/BlockCount(0x%x) invalid in LVM_AddVolume",
			BlockSize, BlockCount);
		return NULL;
	}

	dummy_vol.Type = Type;
	dummy_vol.Ptr = Ptr;
	dummy_vol.BlockCount = BlockCount;
	dummy_vol.BlockSize = BlockSize;
	dummy_vol.CacheHandle = NULL;

	// Read the first block of the volume	
	void *first_block = malloc(BlockSize);
	if( !first_block ) {
		Log_Error("LVM", "LVM_AddVolume - malloc error on %i bytes", BlockSize);
		return NULL;
	}
	if( Type->Read(Ptr, 0, 1, first_block) != 1 ) {
		Log_Error("LVM", "LVM_AddVolume - Failed to read first sector");
		free(first_block);
		return NULL;
	}
	
	// Determine Format
	// TODO: Determine format
	fmt = &gLVM_MBRType;

	// Type->CountSubvolumes
	dummy_vol.nSubVolumes = fmt->CountSubvolumes(&dummy_vol, first_block);
	Log_Debug("LVM", "Volume %s as format %s gives %i sub-volumes",
		Name, fmt->Name, dummy_vol.nSubVolumes);
	
	// Create real volume descriptor
	// TODO: If this needs to be rescanned later, having the subvolume list separate might be an idea
	size_t	allocsize = sizeof(tLVM_Vol) + strlen(Name) + 1 + sizeof(tLVM_SubVolume*) * dummy_vol.nSubVolumes;
	tLVM_Vol	*real_vol = malloc( allocsize );
	if( !real_vol ) {
		Log_Error("LVM", "LVM_AddVolume - malloc error on %i bytes", allocsize);
		free(first_block);
		return NULL;
	}
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

	// TODO: Better selection of cache size
	// TODO: Allow a volume type to disallow caching
	#if USE_IOCACHE
	real_vol->CacheHandle = IOCache_Create(LVM_int_CacheWriteback, real_vol, BlockSize, 1024);
	#else
	real_vol->CacheHandle = NULL;
	#endif

	// Type->PopulateSubvolumes
	fmt->PopulateSubvolumes(real_vol, first_block);
	free(first_block);

	// Add to volume list
	gpLVM_LastVolume->Next = real_vol;
	gpLVM_LastVolume = real_vol;

	return real_vol;
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
int LVM_int_CacheWriteback(void *ID, Uint64 Sector, const void *Buffer)
{
	tLVM_Vol *Volume = ID;
	return Volume->Type->Write(Volume->Ptr, Sector, 1, Buffer);
}

size_t LVM_int_ReadVolume(tLVM_Vol *Volume, Uint64 BlockNum, size_t BlockCount, void *Dest)
{
	#if USE_IOCACHE
	if( Volume->CacheHandle )
	{
		 int	done = 0;
		while( done < BlockCount )
		{
			while( done < BlockCount && IOCache_Read(Volume->CacheHandle, BlockNum+done, Dest) == 1 )
				done ++, Dest = (char*)Dest + Volume->BlockSize;
			size_t first_uncached = done;
			void *uncache_buf = Dest;
			LOG("%i/%i: cached", done, BlockCount);
			while( done < BlockCount && IOCache_Read(Volume->CacheHandle, BlockNum+done, Dest) == 0 )
				done ++, Dest = (char*)Dest + Volume->BlockSize;
			LOG("%i/%i: uncached", done, BlockCount);
			size_t	count = done-first_uncached;
			if( count ) {
				Volume->Type->Read(Volume->Ptr, BlockNum+first_uncached, count, uncache_buf);
				while(count--)
				{
					IOCache_Add(Volume->CacheHandle, BlockNum+first_uncached, uncache_buf);
					first_uncached ++;
					uncache_buf = (char*)uncache_buf + Volume->BlockSize;
				}
			}
		}
		return done;
	}
	else
	#endif
		return Volume->Type->Read(Volume->Ptr, BlockNum, BlockCount, Dest);
}

size_t LVM_int_WriteVolume(tLVM_Vol *Volume, Uint64 BlockNum, size_t BlockCount, const void *Src)
{
	#if USE_IOCACHE
	if( Volume->CacheHandle )
	{
		int done = 0;
		while( BlockCount )
		{
			IOCache_Write(Volume->CacheHandle, BlockNum, Src);
			Src = (const char*)Src + Volume->BlockSize;
			BlockNum ++;
			BlockCount --;
			done ++;
		}
		return done;
	}
	else
	#endif
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

