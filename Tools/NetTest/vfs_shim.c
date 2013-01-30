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
int VFS_SelectNode(tVFS_Node *Node, int Type, tTime *Timeout, const char *Name)
{
	
	return 0;
}

int VFS_MarkAvaliable(tVFS_Node *Node, BOOL bAvail)
{
	Node->DataAvaliable = bAvail;
	if( Node->DataAvaliable && Node->ReadThreads )
		Threads_PostEvent( (void*)Node->ReadThreads, THREAD_EVENT_VFS );
	return 0;
}

int VFS_MarkError(tVFS_Node *Node, BOOL bError)
{
	Node->ErrorOccurred = bError;
	if( Node->ErrorOccurred && Node->ErrorThreads )
		Threads_PostEvent( (void*)Node->ErrorThreads, THREAD_EVENT_VFS );
	return 0;
}

#if 0
int VFS_Open(const char *Path, Uint Flags)
{
	return -1;
}

void VFS_Close(int FD)
{
}
#endif

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
