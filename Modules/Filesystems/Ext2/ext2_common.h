/*
 * Acess OS
 * Ext2 Driver Version 1
 */
/**
 * \file ext2_common.h
 * \brief Second Extended Filesystem Driver
 */
#ifndef _EXT2_COMMON_H
#define _EXT2_COMMON_H
#include <acess.h>
#include <vfs.h>
#include "ext2fs.h"

#define EXT2_UPDATE_WRITEBACK	1

// === STRUCTURES ===
typedef struct {
	 int	FD;
	 int	CacheID;
	tVFS_Node	RootNode;
	
	tExt2_SuperBlock	SuperBlock;
	 int	BlockSize;
	 
	 int	GroupCount;
	tExt2_Group		Groups[];
} tExt2_Disk;

// === FUNCTIONS ===
// --- Common ---
extern void	Ext2_CloseFile(tVFS_Node *Node);
extern Uint64	Ext2_int_GetBlockAddr(tExt2_Disk *Disk, Uint32 *Blocks, int BlockNum);
extern void	Ext2_int_UpdateSuperblock(tExt2_Disk *Disk);
extern int	Ext2_int_ReadInode(tExt2_Disk *Disk, Uint32 InodeId, tExt2_Inode *Inode);
extern int	Ext2_int_WriteInode(tExt2_Disk *Disk, Uint32 InodeId, tExt2_Inode *Inode);
// --- Dir ---
extern char	*Ext2_ReadDir(tVFS_Node *Node, int Pos);
extern tVFS_Node	*Ext2_FindDir(tVFS_Node *Node, char *FileName);
extern int	Ext2_MkNod(tVFS_Node *Node, char *Name, Uint Flags);
// --- Read ---
extern Uint64	Ext2_Read(tVFS_Node *node, Uint64 offset, Uint64 length, void *buffer);
// --- Write ---
extern Uint64	Ext2_Write(tVFS_Node *node, Uint64 offset, Uint64 length, void *buffer);

#endif
