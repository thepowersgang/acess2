/*
 * Acess OS
 * Ext2 Driver Version 1
 */
/**
 * \file write.c
 * \brief Second Extended Filesystem Driver
 * \todo Implement file full write support
 */
#define DEBUG	1
#define VERBOSE	0
#include "ext2_common.h"

// === PROTOYPES ===
Uint64		Ext2_Write(tVFS_Node *node, Uint64 offset, Uint64 length, void *buffer);
Uint32		Ext2_int_AllocateBlock(tExt2_Disk *Disk, Uint32 PrevBlock);

// === CODE ===
/**
 * \fn Uint64 Ext2_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Write to a file
 */
Uint64 Ext2_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tExt2_Disk	*disk = Node->ImplPtr;
	tExt2_Inode	inode;
	Uint64	base;
	Uint64	retLen;
	Uint	block;
	Uint64	allocSize;
	 int	bNewBlocks = 0;
	
	Debug_HexDump("Ext2_Write", Buffer, Length);
	
	Ext2_int_GetInode(Node, &inode);
	
	// Get the ammount of space already allocated
	// - Round size up to block size
	// - block size is a power of two, so this will work
	allocSize = (inode.i_size + disk->BlockSize) & ~(disk->BlockSize-1);
	
	// Are we writing to inside the allocated space?
	if( Offset < allocSize )
	{
		// Will we go out of it?
		if(Offset + Length > allocSize) {
			bNewBlocks = 1;
			retLen = allocSize - Offset;
		} else
			retLen = Length;
		
		// Within the allocated space
		block = Offset / disk->BlockSize;
		Offset %= disk->BlockSize;
		base = Ext2_int_GetBlockAddr(disk, inode.i_block, block);
		
		// Write only block (if only one)
		if(Offset + retLen <= disk->BlockSize) {
			VFS_WriteAt(disk->FD, base+Offset, retLen, Buffer);
			if(!bNewBlocks)	return Length;
			goto addBlocks;	// Ugh! A goto, but it seems unavoidable
		}
		
		// Write First Block
		VFS_WriteAt(disk->FD, base+Offset, disk->BlockSize-Offset, Buffer);
		Buffer += disk->BlockSize-Offset;
		retLen -= disk->BlockSize-Offset;
		block ++;
		
		// Write middle blocks
		while(retLen > disk->BlockSize)
		{
			base = Ext2_int_GetBlockAddr(disk, inode.i_block, block);
			VFS_WriteAt(disk->FD, base, disk->BlockSize, Buffer);
			Buffer += disk->BlockSize;
			retLen -= disk->BlockSize;
			block ++;
		}
		
		// Write last block
		base = Ext2_int_GetBlockAddr(disk, inode.i_block, block);
		VFS_WriteAt(disk->FD, base, retLen, Buffer);
		if(!bNewBlocks)	return Length;	// Writing in only allocated space
	}
	
addBlocks:
	///\todo Implement block allocation
	Warning("[EXT2] File extending is not yet supported");
	
	return 0;
}

/**
 * \fn Uint32 Ext2_int_AllocateBlock(tExt2_Disk *Disk, Uint32 PrevBlock)
 * \brief Allocate a block from the best possible location
 * \param Disk	EXT2 Disk Information Structure
 * \param PrevBlock	Previous block ID in the file
 */
Uint32 Ext2_int_AllocateBlock(tExt2_Disk *Disk, Uint32 PrevBlock)
{
	 int	bpg = Disk->SuperBlock.s_blocks_per_group;
	Uint	blockgroup = PrevBlock / bpg;
	Uint	bitmap[Disk->BlockSize/sizeof(Uint)];
	Uint	bitsperblock = 8*Disk->BlockSize;
	 int	i, j = 0;
	Uint	block;
	
	// Are there any free blocks?
	if(Disk->SuperBlock.s_free_blocks_count == 0)	return 0;
	
	if(Disk->Groups[blockgroup].bg_free_blocks_count > 0)
	{
		// Search block group's bitmap
		for(i = 0; i < bpg; i++)
		{
			// Get the block in the bitmap block
			j = i & (bitsperblock-1);
			
			// Read in if needed
			if(j == 0) {
				VFS_ReadAt(
					Disk->FD,
					(Uint64)Disk->Groups[blockgroup].bg_block_bitmap + i / bitsperblock,
					Disk->BlockSize,
					bitmap
					);
			}
			
			// Fast Check
			if( bitmap[j/32] == -1 ) {
				j = (j + 31) & ~31;
				continue;
			}
			
			// Is the bit set?
			if( bitmap[j/32] & (1 << (j%32)) )
				continue;
			
			// Ooh! We found one
			break;
		}
		if( i < bpg ) {
			Warning("[EXT2 ] Inconsistency detected, Group Free Block count is non-zero when no free blocks exist");
			goto	checkAll;	// Search the entire filesystem for a free block
			// Goto needed for neatness
		}
		
		// Mark as used
		bitmap[j/32] |= (1 << (j%32));
		VFS_WriteAt(
			Disk->FD,
			(Uint64)Disk->Groups[blockgroup].bg_block_bitmap + i / bitsperblock,
			Disk->BlockSize,
			bitmap
			);
		block = i;
		Disk->Groups[blockgroup].bg_free_blocks_count --;
		#if EXT2_UPDATE_WRITEBACK
		//Ext2_int_UpdateBlockGroup(blockgroup);
		#endif
	}
	else
	{
	checkAll:
		Warning("[EXT2 ] TODO - Implement using blocks outside the current block group");
		return 0;
	}
	
	// Reduce global count
	Disk->SuperBlock.s_free_blocks_count --;
	#if EXT2_UPDATE_WRITEBACK
	Ext2_int_UpdateSuperblock(Disk);
	#endif
	
	return block;
}
