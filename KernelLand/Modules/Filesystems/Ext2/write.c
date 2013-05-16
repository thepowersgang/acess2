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
Uint32		Ext2_int_AllocateBlock(tExt2_Disk *Disk, Uint32 PrevBlock);
void	Ext2_int_DeallocateBlock(tExt2_Disk *Disk, Uint32 Block);
 int	Ext2_int_AppendBlock(tExt2_Disk *Disk, tExt2_Inode *Inode, Uint32 Block);

// === CODE ===
/**
 * \brief Write to a file
 */
size_t Ext2_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags)
{
	tExt2_Disk	*disk = Node->ImplPtr;
	tExt2_Inode	inode;
	Uint64	base;
	Uint64	retLen;
	Uint	block;
	Uint64	allocSize;
	 int	bNewBlocks = 0;
	
	Debug_HexDump("Ext2_Write", Buffer, Length);

	// TODO: Handle (Flags & VFS_IOFLAG_NOBLOCK)	

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
		if( Ext2_int_AppendBlock(disk, &inode, block) ) {
			Log_Warning("Ext2", "Appending %x to inode %p:%X failed",
				block, disk, Node->Inode);
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
	if( Ext2_int_AppendBlock(disk, &inode, block) ) {
		Log_Warning("Ext2", "Appending %x to inode %p:%X failed",
			block, disk, Node->Inode);
		Ext2_int_DeallocateBlock(disk, block);
		goto ret;
	}
	base = block * disk->BlockSize;
	VFS_WriteAt(disk->FD, base, retLen, Buffer);
	
	// TODO: When should the size update be committed?
	inode.i_size += retLen;
	Node->Size += retLen;
	Node->Flags |= VFS_FFLAG_DIRTY;
	
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
	Uint	firstgroup = PrevBlock / bpg;
	Uint	blockgroup = firstgroup;
	tExt2_Group	*bg;

	// TODO: Need to do locking on the bitmaps	

	// Are there any free blocks?
	if(Disk->SuperBlock.s_free_blocks_count == 0)
		return 0;

	// First: Check the next block after \a PrevBlock
	if( (PrevBlock + 1) % Disk->SuperBlock.s_blocks_per_group != 0
	 && Disk->Groups[blockgroup].bg_free_blocks_count > 0 )
	{
		bg = &Disk->Groups[blockgroup];
		const int sector_size = 512;
		Uint8 buf[sector_size];
		 int	iblock = (PrevBlock + 1) % Disk->SuperBlock.s_blocks_per_group;
		 int	byte = iblock / 8;
		 int	ofs = byte / sector_size * sector_size;
		byte %= sector_size;
		VFS_ReadAt(Disk->FD, Disk->BlockSize*bg->bg_block_bitmap+ofs, sector_size, buf);
		
		if( (buf[byte] & (1 << (iblock%8))) == 0 )
		{
			// Free block - nice and contig allocation
			buf[byte] |= (1 << (iblock%8));
			VFS_WriteAt(Disk->FD, Disk->BlockSize*bg->bg_block_bitmap+ofs, sector_size, buf);

			bg->bg_free_blocks_count --;
			Disk->SuperBlock.s_free_blocks_count --;
			#if EXT2_UPDATE_WRITEBACK
			Ext2_int_UpdateSuperblock(Disk);
			#endif
			return PrevBlock + 1;
		}
		// Used... darnit
		// Fall through and search further
	}

	// Second: Search for a group with free blocks
	while( blockgroup < Disk->GroupCount && Disk->Groups[blockgroup].bg_free_blocks_count == 0 )
		blockgroup ++;
	if( Disk->Groups[blockgroup].bg_free_blocks_count == 0 )
	{
		blockgroup = 0;
		while( blockgroup < firstgroup && Disk->Groups[blockgroup].bg_free_blocks_count == 0 )
			blockgroup ++;
	}
	if( Disk->Groups[blockgroup].bg_free_blocks_count == 0 ) {
		Log_Notice("Ext2", "Ext2_int_AllocateBlock - Out of blockss on %p, but superblock says some free",
			Disk);
		return 0;
	}

	// Search the bitmap for a free block
	bg = &Disk->Groups[blockgroup];	
	 int	ofs = 0;
	do {
		const int sector_size = 512;
		Uint8 buf[sector_size];
		VFS_ReadAt(Disk->FD, Disk->BlockSize*bg->bg_block_bitmap+ofs, sector_size, buf);

		int byte, bit;
		for( byte = 0; byte < sector_size && buf[byte] != 0xFF; byte ++ )
			;
		if( byte < sector_size )
		{
			for( bit = 0; bit < 8 && buf[byte] & (1 << bit); bit ++)
				;
			ASSERT(bit != 8);
			buf[byte] |= 1 << bit;
			VFS_WriteAt(Disk->FD, Disk->BlockSize*bg->bg_block_bitmap+ofs, sector_size, buf);

			bg->bg_free_blocks_count --;
			Disk->SuperBlock.s_free_blocks_count --;

			#if EXT2_UPDATE_WRITEBACK
			Ext2_int_UpdateSuperblock(Disk);
			#endif

			Uint32	ret = blockgroup * Disk->SuperBlock.s_blocks_per_group + byte * 8 + bit;
			Log_Debug("Ext2", "Ext2_int_AllocateBlock - Allocated 0x%x", ret);
			return ret;
		}
	} while(ofs < Disk->SuperBlock.s_blocks_per_group / 8);
	
	Log_Notice("Ext2", "Ext2_int_AllocateBlock - Out of block in group %p:%i but header reported free",
		Disk, blockgroup);
	return 0;
}

/**
 * \brief Deallocates a block
 */
void Ext2_int_DeallocateBlock(tExt2_Disk *Disk, Uint32 Block)
{
	Log_Warning("Ext2", "TODO: Impliment Ext2_int_DeallocateBlock");
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
				Log_Warning("Ext2", "Allocating indirect block failed");
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
				Log_Warning("Ext2", "Allocating double indirect block failed");
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
				Log_Warning("Ext2", "Allocating double indirect block (l2) failed");
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
				Log_Warning("Ext2", "Allocating triple indirect block failed");
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
				Log_Warning("Ext2", "Allocating triple indirect block (l2) failed");
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
				Log_Warning("Ext2", "Allocating triple indirect block (l3) failed");
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
	
	Log_Warning("Ext2", "Inode ?? cannot have a block appended to it, all indirects used");
	free(blocks);
	return 1;
}
