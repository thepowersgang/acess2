/*
 * Acess OS
 * Ext2 Driver Version 1
 */
/**
 * \file read.c
 * \brief Second Extended Filesystem Driver
 * \todo Implement file full write support
 */
#define DEBUG	0
#define VERBOSE	0
#include "ext2_common.h"

// === CODE ===
/**
 * \brief Read from a file
 */
size_t Ext2_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
{
	tExt2_Disk	*disk = Node->ImplPtr;
	tExt2_Inode	*inode = (void*)(Node+1);
	Uint64	base;
	Uint	block;
	Uint64	remLen;
	
	ENTER("pNode XOffset xLength pBuffer", Node, Offset, Length, Buffer);
	
	// Sanity Checks
	if(Offset >= inode->i_size) {
		LEAVE('i', 0);
		return 0;
	}
	if(Offset + Length > inode->i_size)
		Length = inode->i_size - Offset;
	
	block = Offset / disk->BlockSize;
	Offset = Offset % disk->BlockSize;
	base = Ext2_int_GetBlockAddr(disk, inode->i_block, block);
	if(base == 0) {
		Log_Warning("EXT2", "NULL Block Detected in INode 0x%llx (Block %i)", Node->Inode, block);
		LEAVE('i', 0);
		return 0;
	}
	
	// Read only block
	if(Length <= disk->BlockSize - Offset)
	{
		VFS_ReadAt( disk->FD, base+Offset, Length, Buffer);
		LEAVE('X', Length);
		return Length;
	}
	
	// Read first block
	// TODO: If (Flags & VFS_IOFLAG_NOBLOCK) trigger read and return EWOULDBLOCK?
	remLen = Length;
	VFS_ReadAt( disk->FD, base + Offset, disk->BlockSize - Offset, Buffer);
	remLen -= disk->BlockSize - Offset;
	Buffer += disk->BlockSize - Offset;
	block ++;
	
	// Read middle blocks
	while(remLen > disk->BlockSize)
	{
		base = Ext2_int_GetBlockAddr(disk, inode->i_block, block);
		if(base == 0) {
			Log_Warning("EXT2", "NULL Block Detected in INode 0x%llx", Node->Inode);
			LEAVE('i', 0);
			return 0;
		}
		VFS_ReadAt( disk->FD, base, disk->BlockSize, Buffer);
		Buffer += disk->BlockSize;
		remLen -= disk->BlockSize;
		block ++;
	}
	
	// Read last block
	base = Ext2_int_GetBlockAddr(disk, inode->i_block, block);
	VFS_ReadAt( disk->FD, base, remLen, Buffer);
	
	LEAVE('X', Length);
	return Length;
}
