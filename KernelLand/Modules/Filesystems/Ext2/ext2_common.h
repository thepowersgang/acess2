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
	Uint	BlockSize;
	 
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
extern tVFS_Node	*Ext2_FindDir(tVFS_Node *Node, const char *FileName);
extern int	Ext2_MkNod(tVFS_Node *Node, const char *Name, Uint Flags);
extern int	Ext2_Link(tVFS_Node *Parent, const char *Name, tVFS_Node *Node);
// --- Read ---
extern size_t	Ext2_Read(tVFS_Node *node, off_t offset, size_t length, void *buffer);
// --- Write ---
extern size_t	Ext2_Write(tVFS_Node *node, off_t offset, size_t length, const void *buffer);

#endif
