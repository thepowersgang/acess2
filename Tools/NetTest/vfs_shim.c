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

int VFS_Open(const char *Path, Uint Flags)
{
	return -1;
}

void VFS_Close(int FD)
{
}

tVFS_Handle *VFS_GetHandle(int FD)
{
	return NULL;
}

