/*
 * Acess2 Networking Test Suite (NetTest)
 * - By John Hodge (thePowersGang)
 *
 * vfs_shim.c
 * - VFS Layer Emulation
 */
#include <vfs.h>
#include <vfs_int.h>
#include <events.h>

// === CODE ===
int VFS_AllocHandle(int bIsUser, tVFS_Node *Node, int Mode)
{
	const int maxfd = *Threads_GetMaxFD();
	tVFS_Handle	*handles = *Threads_GetHandlesPtr();
	if( !handles ) {
		handles = calloc( maxfd, sizeof(tVFS_Handle) );
		*Threads_GetHandlesPtr() = handles;
	}

	// TODO: Global handles

	for( int i = 0; i < maxfd; i ++ )
	{
		if( handles[i].Node == NULL ) {
			handles[i].Node = Node;
			handles[i].Mode = Mode;
			return i;
		}
	}
	return -1;
}

tVFS_Handle *VFS_GetHandle(int FD)
{
	const int maxfd = *Threads_GetMaxFD();
	tVFS_Handle	*handles = *Threads_GetHandlesPtr();
	if( !handles )
		return NULL;

	if( FD < 0 || FD >= maxfd )
		return NULL;

	return &handles[FD];
}

int VFS_SetHandle(int FD, tVFS_Node *Node, int Mode)
{
	const int maxfd = *Threads_GetMaxFD();
	tVFS_Handle	*handles = *Threads_GetHandlesPtr();
	if( !handles )
		return -1;

	if( FD < 0 || FD >= maxfd )
		return -1;

	handles[FD].Node = Node;
	handles[FD].Mode = Mode;
	return FD;
}
