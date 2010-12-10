/*
 * Acess2 VFS
 * - AllocHandle, GetHandle
 */
#define DEBUG	0
#include <acess.h>
#include "vfs.h"
#include "vfs_int.h"
#include "vfs_ext.h"

// === CONSTANTS ===
#define MAX_KERNEL_FILES	128
#define MAX_USER_FILES	64

// === PROTOTYPES ===
tVFS_Handle	*VFS_GetHandle(int FD);
 int	VFS_AllocHandle(int FD, tVFS_Node *Node, int Mode);

typedef struct sUserHandles
{
	struct sUserHandles	*Next;
	 int	PID;
	tVFS_Handle	Handles[MAX_USER_FILES];
}	tUserHandles;

// === GLOBALS ===
tUserHandles	*gpUserHandles = NULL;
tVFS_Handle	gaKernelHandles[MAX_KERNEL_FILES];

// === CODE ===
/**
 * \fn tVFS_Handle *VFS_GetHandle(int FD)
 * \brief Gets a pointer to the handle information structure
 */
tVFS_Handle *VFS_GetHandle(int FD)
{
	tVFS_Handle	*h;
	
	//Log_Debug("VFS", "VFS_GetHandle: (FD=0x%x)", FD);
	
	if(FD < 0)	return NULL;
	
	if(FD & VFS_KERNEL_FLAG) {
		FD &= (VFS_KERNEL_FLAG - 1);
		if(FD >= MAX_KERNEL_FILES)	return NULL;
		h = &gaKernelHandles[ FD ];
	}
	else {
		tUserHandles	*ent;
		 int	pid = Threads_GetPID();
		for( ent = gpUserHandles; ent; ent = ent->Next ) {
			if( ent->PID == pid )	break;
			if( ent->PID > pid ) {
				Log_Error("VFS", "PID %i does not have a handle list", pid);
				return NULL;
			}
		}
		if(FD >= CFGINT(CFG_VFS_MAXFILES))	return NULL;
		h = &ent->Handles[ FD ];
	}
	
	if(h->Node == NULL)	return NULL;
	//Log_Debug("VFS", "VFS_GetHandle: RETURN %p", h);
	return h;
}

int VFS_AllocHandle(int bIsUser, tVFS_Node *Node, int Mode)
{
	 int	i;
	
	// Check for a user open
	if(bIsUser)
	{
		tUserHandles	*ent, *prev;
		 int	pid = Threads_GetPID();
		for( ent = gpUserHandles; ent; prev = ent, ent = ent->Next ) {
			if( ent->PID == pid )	break;
			if( ent->PID > pid )	break;
		}
		if( ent->PID > pid ) {
			ent = calloc( 1, sizeof(tUserHandles) );
			if( prev ) {
				ent->Next = prev->Next;
				prev->Next = ent;
			}
			else {
				ent->Next = gpUserHandles;
				gpUserHandles = ent;
			}
		}
		// Get a handle
		for(i=0;i<CFGINT(CFG_VFS_MAXFILES);i++)
		{
			if(ent->Handles[i].Node)	continue;
			ent->Handles[i].Node = Node;
			ent->Handles[i].Position = 0;
			ent->Handles[i].Mode = Mode;
			return i;
		}
	}
	else
	{
		// Get a handle
		for(i=0;i<MAX_KERNEL_FILES;i++)
		{
			if(gaKernelHandles[i].Node)	continue;
			gaKernelHandles[i].Node = Node;
			gaKernelHandles[i].Position = 0;
			gaKernelHandles[i].Mode = Mode;
			return i|VFS_KERNEL_FLAG;
		}
	}
	
	return -1;
}
