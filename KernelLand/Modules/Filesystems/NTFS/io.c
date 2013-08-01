/*
 * Acess2 - NTFS Driver
 * - By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess licence. See the
 * file COPYING for details.
 *
 * io.c
 * - File Read/Write
 */
#define DEBUG	1
#include "common.h"

// == CODE ===
size_t NTFS_ReadFile(tVFS_Node *Node, Uint64 Offset, size_t Length, void *Buffer, Uint Flags)
{
	tNTFS_File	*File = (void*)Node;

	ENTER("XNode->Inode XOffset xLength pBuffer xFlags",
		Node->Inode, Offset, Length, Buffer, Flags);	

	size_t	ret =  NTFS_ReadAttribData(File->Data, Offset, Length, Buffer);
	LEAVE('x', ret);
	return ret;
}


