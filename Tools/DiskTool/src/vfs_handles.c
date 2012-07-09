/*
 * 
 */
#include <acess.h>
#include <vfs.h>
#include <vfs_int.h>

#define MAX_KERNEL_FILES	32

// === GLOBALS ===
tVFS_Handle	gaKernelHandles[MAX_KERNEL_FILES];

// === CODE ===
int VFS_AllocHandle(int bIsUser, tVFS_Node *Node, int Mode)
{
	for( int i = 0; i < MAX_KERNEL_FILES; i ++ )
	{
		if(gaKernelHandles[i].Node)	continue;
		gaKernelHandles[i].Node = Node;
		gaKernelHandles[i].Position = 0;
		gaKernelHandles[i].Mode = Mode;
		return i;
	}	

	return -1;
}

tVFS_Handle *VFS_GetHandle(int ID)
{
	if( ID < 0 || ID >= MAX_KERNEL_FILES )
		return NULL;
	return &gaKernelHandles[ID];
}
