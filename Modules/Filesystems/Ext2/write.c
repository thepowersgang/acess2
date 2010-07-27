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
void	Ext2_int_DeallocateBlock(tExt2_Disk *Disk, Uint32 Block);
 int	Ext2_int_AppendBlock(tExt2_Disk *Disk, tExt2_Inode *Inode, Uint32 Block);

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
	
	Ext2_int_ReadInode(disk, Node->Inode, &inode);
	
	// Get the ammount of space already allocated
	// - Round size up to block size
	// - block size is a power of two, so this will work
	allocSize = (inode.i_size + disk->BlockSize-1) & ~(disk->BlockSize-1);
	
	// Are we writing to inside the allocated space?
	if( Offset > allocSize )	return 0;
	
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
	else
		base = Ext2_int_GetBlockAddr(disk, inode.i_block, allocSize/disk->BlockSize-1);
	
addBlocks:
	Log_Notice("EXT2", "File extending is untested");
	
	// Allocate blocks and copy data to them
	retLen = Length - (allocSize-Offset);
	while( retLen > disk->BlockSize )
	{
		// Allocate a block
		block = Ext2_int_AllocateBlock(disk, base/disk->BlockSize);
		if(!block)	return Length - retLen;
		// Add it to this inode
		if( !Ext2_int_AppendBlock(disk, &inode, block) ) {
			Ext2_int_DeallocateBlock(disk, block);
			goto ret;
		}
		// Copy data to the node
		base = block * disk->BlockSize;
		VFS_WriteAt(disk->FD, base, disk->BlockSize, Buffer);
		// Update pointer and size remaining
		inode.i_size += disk->BlockSize;
		Buffer += disk->BlockSize;
		retLen -= disk->BlockSize;
	}
	// Last block :D
	block = Ext2_int_AllocateBlock(disk, base/disk->BlockSize);
	if(!block)	goto ret;
	if( !Ext2_int_AppendBlock(disk, &inode, block) ) {
		Ext2_int_DeallocateBlock(disk, block);
		goto ret;
	}
	base = block * disk->BlockSize;
	VFS_WriteAt(disk->FD, base, retLen, Buffer);
	inode.i_size += retLen;
	retLen = 0;

ret:	// Makes sure the changes to the inode are committed
	Ext2_int_WriteInode(disk, Node->Inode, &inode);
	return Length - retLen;
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
			if( bitmap[j/32] == 0xFFFFFFFF ) {
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
		//Ext2_int_UpdateBlockGroup(Disk, blockgroup);
		#endif
	}
	else
	{
	checkAll:
		Log_Warning("EXT2", "TODO - Implement using blocks outside the current block group");
		return 0;
	}
	
	// Reduce global count
	Disk->SuperBlock.s_free_blocks_count --;
	#if EXT2_UPDATE_WRITEBACK
	Ext2_int_UpdateSuperblock(Disk);
	#endif
	
	return block;
}

/**
 * \brief Deallocates a block
 */
void Ext2_int_DeallocateBlock(tExt2_Disk *Disk, Uint32 Block)
{
}

/**
 * \brief Append a block to an inode
 */
int Ext2_int_AppendBlock(tExt2_Disk *Disk, tExt2_Inode *Inode, Uint32 Block)
{
	 int	nBlocks;
	 int	dwPerBlock = Disk->BlockSize / 4;
	Uint32	*blocks;
	Uint32	id1, id2;
	
	nBlocks = (Inode->i_size + Disk->BlockSize - 1) / Disk->BlockSize;
	
	// Direct Blocks
	if( nBlocks < 12 ) {
		Inode->i_block[nBlocks] = Block;
		return 0;
	}
	
	blocks = malloc( Disk->BlockSize );
	if(!blocks)	return 1;
	
	nBlocks -= 12;
	// Single Indirect
	if( nBlocks < dwPerBlock)
	{
		// Allocate/Get Indirect block
		if( nBlocks == 0 ) {
			Inode->i_block[12] = Ext2_int_AllocateBlock(Disk, Inode->i_block[0]);
			if( !Inode->i_block[12] ) {
				free(blocks);
				return 1;
			}
			memset(blocks, 0, Disk->BlockSize); 
		}
		else
			VFS_ReadAt(Disk->FD, Inode->i_block[12]*Disk->BlockSize, Disk->BlockSize, blocks);
		
		blocks[nBlocks] = Block;
		
		VFS_WriteAt(Disk->FD, Inode->i_block[12]*Disk->BlockSize, Disk->BlockSize, blocks);
		free(blocks);
		return 0;
	}
	
	nBlocks += dwPerBlock;
	// Double Indirect
	if( nBlocks < dwPerBlock*dwPerBlock )
	{
		// Allocate/Get Indirect block
		if( nBlocks == 0 ) {
			Inode->i_block[13] = Ext2_int_AllocateBlock(Disk, Inode->i_block[0]);
			if( !Inode->i_block[13] ) {
				free(blocks);
				return 1;
			}
			memset(blocks, 0, Disk->BlockSize);
		}
		else
			VFS_ReadAt(Disk->FD, Inode->i_block[13]*Disk->BlockSize, Disk->BlockSize, blocks);
		
		// Allocate / Get Indirect lvl2 Block
		if( nBlocks % dwPerBlock == 0 ) {
			id1 = Ext2_int_AllocateBlock(Disk, Inode->i_block[0]);
			if( !id1 ) {
				free(blocks);
				return 1;
			}
			blocks[nBlocks/dwPerBlock] = id1;
			// Write back indirect 1 block
			VFS_WriteAt(Disk->FD, Inode->i_block[13]*Disk->BlockSize, Disk->BlockSize, blocks);
			memset(blocks, 0, Disk->BlockSize);
		}
		else {
			id1 = blocks[nBlocks / dwPerBlock];
			VFS_ReadAt(Disk->FD, id1*Disk->BlockSize, Disk->BlockSize, blocks);
		}
		
		blocks[nBlocks % dwPerBlock] = Block;
		
		VFS_WriteAt(Disk->FD, id1*Disk->BlockSize, Disk->BlockSize, blocks);
		free(blocks);
		return 0;
	}
	
	nBlocks -= dwPerBlock*dwPerBlock;
	// Triple Indirect
	if( nBlocks < dwPerBlock*dwPerBlock*dwPerBlock )
	{
		// Allocate/Get Indirect block
		if( nBlocks == 0 ) {
			Inode->i_block[14] = Ext2_int_AllocateBlock(Disk, Inode->i_block[0]);
			if( !Inode->i_block[14] ) {
				free(blocks);
				return 1;
			}
			memset(blocks, 0, Disk->BlockSize);
		}
		else
			VFS_ReadAt(Disk->FD, Inode->i_block[14]*Disk->BlockSize, Disk->BlockSize, blocks);
		
		// Allocate / Get Indirect lvl2 Block
		if( (nBlocks/dwPerBlock) % dwPerBlock == 0 && nBlocks % dwPerBlock == 0 )
		{
			id1 = Ext2_int_AllocateBlock(Disk, Inode->i_block[0]);
			if( !id1 ) {
				free(blocks);
				return 1;
			}
			blocks[nBlocks/dwPerBlock] = id1;
			// Write back indirect 1 block
			VFS_WriteAt(Disk->FD, Inode->i_block[14]*Disk->BlockSize, Disk->BlockSize, blocks);
			memset(blocks, 0, Disk->BlockSize);
		}
		else {
			id1 = blocks[nBlocks / (dwPerBlock*dwPerBlock)];
			VFS_ReadAt(Disk->FD, id1*Disk->BlockSize, Disk->BlockSize, blocks);
		}
		
		// Allocate / Get Indirect Level 3 Block
		if( nBlocks % dwPerBlock == 0 ) {
			id2 = Ext2_int_AllocateBlock(Disk, id1);
			if( !id2 ) {
				free(blocks);
				return 1;
			}
			blocks[(nBlocks/dwPerBlock)%dwPerBlock] = id2;
			// Write back indirect 1 block
			VFS_WriteAt(Disk->FD, id1*Disk->BlockSize, Disk->BlockSize, blocks);
			memset(blocks, 0, Disk->BlockSize);
		}
		else {
			id2 = blocks[(nBlocks/dwPerBlock)%dwPerBlock];
			VFS_ReadAt(Disk->FD, id2*Disk->BlockSize, Disk->BlockSize, blocks);
		}
		
		blocks[nBlocks % dwPerBlock] = Block;
		
		VFS_WriteAt(Disk->FD, id2*Disk->BlockSize, Disk->BlockSize, blocks);
		free(blocks);
		return 0;
	}
	
	Warning("[EXT2 ] Inode %i cannot have a block appended to it, all indirects used");
	free(blocks);
	return 1;
}
