/*
 * Acess2 - NTFS Driver
 * By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess licence. See the
 * file COPYING for details.
 *
 * dir.c - Directory Handling
 */
#include "common.h"
#include "index.h"

// === PROTOTYPES ===
 int	NTFS_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX]);
tVFS_Node	*NTFS_FindDir(tVFS_Node *Node, const char *Name, Uint Flags);
Uint64	NTFS_int_IndexLookup(Uint64 Inode, const char *IndexName, const char *Str);

// === CODE ===
/**
 * \brief Get the name of an indexed directory entry
 */
int NTFS_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX])
{
	return -ENOTIMPL;
}

/**
 * \brief Get an entry from a directory by name
 */
tVFS_Node *NTFS_FindDir(tVFS_Node *Node, const char *Name, Uint Flags)
{
	tNTFS_Disk	*disk = Node->ImplPtr;
	Uint64	inode = NTFS_int_IndexLookup(Node->Inode, "$I30", Name);
	tVFS_Node	node;
	
	if(!inode)	return NULL;
	
	node.Inode = inode;
	
	return Inode_CacheNode(disk->CacheHandle, &node);
}

/**
 * \brief Scans an index for the requested value and returns the associated ID
 */
Uint64 NTFS_int_IndexLookup(Uint64 Inode, const char *IndexName, const char *Str)
{
	return 0;
}
