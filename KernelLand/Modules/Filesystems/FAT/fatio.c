/*
 * Acess2 FAT12/16/32 Driver
 * - By John Hodge (thePowersGang)
 *
 * fatio.c
 * - FAT Manipulation and Cluster IO
 */
#define DEBUG	0
#include <acess.h>
#include <vfs.h>
#include "common.h"

// === CODE ===
/**
 * \fn Uint32 FAT_int_GetFatValue(tFAT_VolInfo *Disk, Uint32 cluster)
 * \brief Fetches a value from the FAT
 */
Uint32 FAT_int_GetFatValue(tFAT_VolInfo *Disk, Uint32 cluster)
{
	Uint32	val = 0;
	Uint32	ofs;
	ENTER("pDisk xCluster", Disk, cluster);
	
	Mutex_Acquire( &Disk->lFAT );
	#if CACHE_FAT
	if( Disk->ClusterCount <= giFAT_MaxCachedClusters )
	{
		val = Disk->FATCache[cluster];
		if(Disk->type == FAT12 && val == EOC_FAT12)	val = GETFATVALUE_EOC;
		if(Disk->type == FAT16 && val == EOC_FAT16)	val = GETFATVALUE_EOC;
		if(Disk->type == FAT32 && val == EOC_FAT32)	val = GETFATVALUE_EOC;
	}
	else
	{
	#endif
		ofs = Disk->bootsect.resvSectCount*512;
		if(Disk->type == FAT12) {
			VFS_ReadAt(Disk->fileHandle, ofs+(cluster/2)*3, 3, &val);
			LOG("3 bytes at 0x%x are (Uint32)0x%x", ofs+(cluster/2)*3, val);
			val = (cluster & 1) ? (val>>12) : (val & 0xFFF);
			if(val == EOC_FAT12)	val = GETFATVALUE_EOC;
		} else if(Disk->type == FAT16) {
			VFS_ReadAt(Disk->fileHandle, ofs+cluster*2, 2, &val);
			if(val == EOC_FAT16)	val = GETFATVALUE_EOC;
		} else {
			VFS_ReadAt(Disk->fileHandle, ofs+cluster*4, 4, &val);
			if(val == EOC_FAT32)	val = GETFATVALUE_EOC;
		}
	#if CACHE_FAT
	}
	#endif /*CACHE_FAT*/
	Mutex_Release( &Disk->lFAT );
	LEAVE('x', val);
	return val;
}

#if SUPPORT_WRITE
/**
 * \brief Allocate a new cluster
 */
Uint32 FAT_int_AllocateCluster(tFAT_VolInfo *Disk, Uint32 Previous)
{
	Uint32	ret = -1;
	
	#if CACHE_FAT
	if( Disk->ClusterCount <= giFAT_MaxCachedClusters )
	{
		 int	bFoundCluster = 0;
		Uint32	eoc;

		switch(Disk->type)
		{
		case FAT12:	eoc = EOC_FAT12;	break;
		case FAT16:	eoc = EOC_FAT16;	break;
		case FAT32:	eoc = EOC_FAT32;	break;
			default:	return 0;
		}
		
		Mutex_Acquire(&Disk->lFAT);
		if( Previous != -1 )
		{
			for(ret = Previous; ret < Disk->ClusterCount; ret++)
			{
				if(Disk->FATCache[ret] != 0) {
					bFoundCluster = 1;
					break;
				}
			}
		}
		if( !bFoundCluster )
		{
			for(ret = 0; ret < Previous; ret++)
			{
				if(Disk->FATCache[ret] == 0) {
					bFoundCluster = 1;
					break;
				}
			}
		}
		
		if(bFoundCluster)
		{ 
			Disk->FATCache[ret] = eoc;
			if( Previous != -1 )
				Disk->FATCache[Previous] = ret;
		}
		else
		{
			ret = 0;
		}
			
		Mutex_Release(&Disk->lFAT);
		LOG("Allocated cluster %x", ret);
		return ret;
	}
	else
	{
	#endif
		Uint32	val = 0;
		Uint32	base = Disk->bootsect.resvSectCount*512;
		 int	block = 0, block_ofs = 0;
		 int	first_block;
		const int	block_size = 512*3;
		const int	ents_per_block_12 = block_size * 2 / 3;	// 1.5 bytes per entry
//		const int	ents_per_block_16 = block_size / 2;	// 2 bytes per entry
//		const int	ents_per_block_32 = block_size / 4;	// 4 bytes per entry
		 int	block_count_12 = DivUp(Disk->ClusterCount, ents_per_block_12);
		Uint8	sector_data[block_size+1];
		sector_data[block_size] = 0;
		
		Mutex_Acquire(&Disk->lFAT);
		switch(Disk->type)
		{
		case FAT12:
			if( Previous != -1 )
				block = Previous / ents_per_block_12;
			else
				block = 0;
			first_block = block;
			
			// Search within the same block as the previous cluster first
			do {
				VFS_ReadAt(Disk->fileHandle, base + block*block_size, block_size, sector_data);
				for( block_ofs = 0; block_ofs < ents_per_block_12; block_ofs ++ )
				{
					Uint32	*valptr = (void*)( sector_data + block_ofs / 2 * 3 );
					 int	bitofs = 12 * (block_ofs % 2);
//					LOG("%i:%i - FAT Ent 0x%03x", block, block_ofs, (*valptr>>bitofs) & 0xFFF);
					if( ((*valptr >> bitofs) & 0xFFF) == 0 ) {
						// Found a free cluster
						*valptr |= EOC_FAT12 << bitofs;
						ret = block * ents_per_block_12 + block_ofs;
						break;
					}
				}
				// Check for early break from the above loop
				if( block_ofs != ents_per_block_12 )
					break;

				// Next block please
				block ++;
				if( block == block_count_12 )
					block = 0;
			} while( block != first_block );
			
			if( ret != 0 )	// TODO: Could cluster 0 be valid?
			{
				// Write back changes to this part of the FAT
				VFS_WriteAt(Disk->fileHandle, base + block, block_size, sector_data);
	
				// Note the new cluster in the chain
				if( Previous != -1 )
				{
					LOG("Updating cluster %x to point to %x (offset %x)", Previous, ret,
						base + (Previous>>1)*3);
					VFS_ReadAt(Disk->fileHandle, base + (Previous>>1)*3, 3, &val); 
					if( Previous & 1 ) {
						val &= 0x000FFF;
						val |= ret << 12;
					}
					else {
						val &= 0xFFF000;
						val |= ret << 0;
					}
					VFS_WriteAt(Disk->fileHandle, base + (Previous>>1)*3, 3, &val);
				}
			}
			break;
		case FAT16:
			Log_Warning("FAT", "TODO: Implement cluster allocation with FAT16");
//			VFS_ReadAt(Disk->fileHandle, ofs+Previous*2, 2, &ret);
//			VFS_WriteAt(Disk->fileHandle, ofs+ret*2, 2, &eoc);
			break;
		case FAT32:
			Log_Warning("FAT", "TODO: Implement cluster allocation with FAT32");
//			VFS_ReadAt(Disk->fileHandle, ofs+Previous*4, 4, &ret);
//			VFS_WriteAt(Disk->fileHandle, ofs+ret*4, 4, &eoc);
			break;
		}
		Mutex_Release(&Disk->lFAT);
		LOG("Allocated cluster %x", ret);
		return ret;
	#if CACHE_FAT
	}
	#endif
}

/**
 * \brief Free's a cluster
 * \return The original contents of the cluster
 */
Uint32 FAT_int_FreeCluster(tFAT_VolInfo *Disk, Uint32 Cluster)
{
	Uint32	ret;

	if( Cluster < 2 || Cluster > Disk->ClusterCount )	// oops?
	{
		Log_Notice("FAT", "Cluster 0x%x is out of range (2 ... 0x%x)", 
			Cluster, Disk->ClusterCount-1);
		return -1;
	}
	
	Mutex_Acquire(&Disk->lFAT);
	#if CACHE_FAT
	if( Disk->ClusterCount <= giFAT_MaxCachedClusters )
	{
		
		ret = Disk->FATCache[Cluster];
		Disk->FATCache[Cluster] = 0;
	}
	else
	{
	#endif
		Uint32	val = 0;
		Uint32	ofs = Disk->bootsect.resvSectCount*512;
		switch(Disk->type)
		{
		case FAT12:
			VFS_ReadAt(Disk->fileHandle, ofs+(Cluster>>1)*3, 3, &val);
			val = LittleEndian32(val);
			if( Cluster & 1 ) {
				ret = (val >> 12) & 0xFFF;
				val &= 0xFFF;
			}
			else {
				ret = val & 0xFFF;
				val &= 0xFFF000;
			}
			val = LittleEndian32(val);
			VFS_WriteAt(Disk->fileHandle, ofs+(Cluster>>1)*3, 3, &val);
			break;
		case FAT16:
			VFS_ReadAt(Disk->fileHandle, ofs+Cluster*2, 2, &ret);
			ret = LittleEndian16(ret);
			val = 0;
			VFS_WriteAt(Disk->fileHandle, ofs+Cluster*2, 2, &val);
			break;
		case FAT32:
			VFS_ReadAt(Disk->fileHandle, ofs+Cluster*4, 4, &ret);
			ret = LittleEndian32(ret);
			val = 0;
			VFS_WriteAt(Disk->fileHandle, ofs+Cluster*2, 4, &val);
			break;
		}
	#if CACHE_FAT
	}
	#endif
	Mutex_Release(&Disk->lFAT);
	LOG("ret = %07x, eoc = %07x", ret, EOC_FAT12);
	if(ret == 0) {
		Log_Notice("FAT", "Cluster 0x%x was already free", Cluster);
		return -1;
	}
	if(Disk->type == FAT12 && ret == EOC_FAT12)	ret = -1;
	if(Disk->type == FAT16 && ret == EOC_FAT16)	ret = -1;
	if(Disk->type == FAT32 && ret == EOC_FAT32)	ret = -1;
	LOG("ret = %07x", ret);
	return ret;
}
#endif

/*
 * ====================
 *      Cluster IO
 * ====================
 */
/**
 * \brief Read a cluster
 * \param Disk	Disk (Volume) to read from
 * \param Length	Length to read
 * \param Buffer	Destination for read data
 */
void FAT_int_ReadCluster(tFAT_VolInfo *Disk, Uint32 Cluster, int Length, void *Buffer)
{
	ENTER("pDisk xCluster iLength pBuffer", Disk, Cluster, Length, Buffer);
	VFS_ReadAt(
		Disk->fileHandle,
		(Disk->firstDataSect + (Cluster-2)*Disk->bootsect.spc )
			* Disk->bootsect.bps,
		Length,
		Buffer
		);
	LEAVE('-');
}

#if SUPPORT_WRITE
/**
 * \brief Write a cluster to disk
 */
void FAT_int_WriteCluster(tFAT_VolInfo *Disk, Uint32 Cluster, const void *Buffer)
{
	ENTER("pDisk xCluster pBuffer", Disk, Cluster, Buffer);
	VFS_WriteAt(
		Disk->fileHandle,
		(Disk->firstDataSect + (Cluster-2)*Disk->bootsect.spc )
			* Disk->bootsect.bps,
		Disk->BytesPerCluster,
		Buffer
		);
	LEAVE('-');
}
#endif
