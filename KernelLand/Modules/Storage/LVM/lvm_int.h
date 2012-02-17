/*
 * Acess2 Logical Volume Manager
 * - By John Hodge (thePowersGang)
 *
 * lvm_int.h
 * - Internal definitions
 */
#ifndef _LVM_LVM_INT_H_
#define _LVM_LVM_INT_H_

#include "lvm.h"
#include <vfs.h>

typedef struct sLVM_SubVolume	tLVM_SubVolume;

struct sLVM_Vol
{
	tLVM_Vol	*Next;
	
	tVFS_Node	Node;
	
	 int	BackingDescriptor;
	size_t	BlockSize;
	
	 int	nSubVolumes;
	tLVM_SubVolume	**SubVolumes;

	char	Name[];
};

struct sLVM_SubVolume
{
	tLVM_Vol	*Vol;
	
	tVFS_Node	Node;

	// Note: Only for a simple volume
	Uint64	FirstBlock;
	Uint64	BlockCount;

	char	Name[];
};

extern tVFS_NodeType	gLVM_SubVolNodeType;

#endif

