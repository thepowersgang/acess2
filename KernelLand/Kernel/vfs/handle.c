/*
 * Acess2 VFS
 * - AllocHandle, GetHandle
 */
#define SANITY	1
#define DEBUG	0
#include <acess.h>
#include <mm_virt.h>
#include "vfs.h"
#include "vfs_int.h"
#include "vfs_ext.h"
#include <threads.h>	// GetMaxFD
#include <vfs_threads.h>	// Handle maintainance

// === CONSTANTS ===
#define MAX_KERNEL_FILES	128

// === PROTOTYPES ===

// === GLOBALS ===
tVFS_Handle	*gaUserHandles = (void*)MM_PPD_HANDLES;
tVFS_Handle	*gaKernelHandles = (void*)MM_KERNEL_VFS;

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
	} else {
		if(FD >= *Threads_GetMaxFD(NULL))	return NULL;
		h = &gaUserHandles[ FD ];
	}
	
	if(h->Node == NULL)	return NULL;
	//Log_Debug("VFS", "VFS_GetHandle: RETURN %p", h);
	return h;
}

int VFS_SetHandle(int FD, tVFS_Node *Node, int Mode)
{
	tVFS_Handle	*h;
	if(FD < 0)	return -1;
	
	if( FD & VFS_KERNEL_FLAG ) {
		FD &= (VFS_KERNEL_FLAG -1);
		if( FD >= MAX_KERNEL_FILES )	return -1;
		h = &gaKernelHandles[FD];
	}
	else {
		if( FD >= *Threads_GetMaxFD(NULL))	return -1;
		h = &gaUserHandles[FD];
	}
	h->Node = Node;
	h->Mode = Mode;
	return FD;
}

int VFS_AllocHandle(int bIsUser, tVFS_Node *Node, int Mode)
{
	// Check for a user open
	if(bIsUser)
	{
		 int	max_handles = *Threads_GetMaxFD(NULL);
		// Allocate Buffer
		if( MM_GetPhysAddr( gaUserHandles ) == 0 )
		{
			tPage	*pageptr = (void*)gaUserHandles;
			size_t	size = max_handles * sizeof(tVFS_Handle);
			for( size_t ofs = 0; ofs < size; ofs ++)
			{
				if( !MM_Allocate( pageptr ) )
				{
					Warning("OOM - VFS_AllocHandle");
					Threads_Exit(0, 0xFF);	// Terminate user
				}
				pageptr ++;
			}
			memset( gaUserHandles, 0, size );
		}
		// Get a handle
		for( int i = 0; i < max_handles; i ++ )
		{
			if(gaUserHandles[i].Node)	continue;
			gaUserHandles[i].Node = Node;
			gaUserHandles[i].Position = 0;
			gaUserHandles[i].Mode = Mode;
			return i;
		}
	}
	else
	{
		// Allocate space if not already
		if( MM_GetPhysAddr( gaKernelHandles ) == 0 )
		{
			tPage	*pageptr = (void*)gaKernelHandles;
			size_t	size = MAX_KERNEL_FILES * sizeof(tVFS_Handle);
			for(size_t ofs = 0; ofs < size; ofs += size)
			{
				if( !MM_Allocate( pageptr ) )
				{
					Panic("OOM - VFS_AllocHandle");
				}
				pageptr ++;
			}
			memset( gaKernelHandles, 0, size );
		}
		// Get a handle
		for(int i=0;i<MAX_KERNEL_FILES;i++)
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

void VFS_ReferenceUserHandles(void)
{
	const int	max_handles = *Threads_GetMaxFD(NULL);

	// Check if this process has any handles
	if( MM_GetPhysAddr( gaUserHandles ) == 0 )
		return ;
	
	for( int i = 0; i < max_handles; i ++ )
	{
		tVFS_Handle	*h;
		h = &gaUserHandles[i];
		if( !h->Node )
			continue ;
		_ReferenceNode(h->Node);
		h->Mount->OpenHandleCount ++;
	}
}

void VFS_CloseAllUserHandles(struct sProcess *Process)
{
	const int	max_handles = *Threads_GetMaxFD(Process);
	ENTER("pProcess", Process);

	if( max_handles >= PAGE_SIZE / sizeof(tVFS_Handle) )
		TODO("More than a page of handles");

	tVFS_Handle	*handles = MM_MapTempFromProc(Process, gaUserHandles);
	LOG("handles=%p", handles);
	// Check if this process has any handles
	if( !handles ) {
		LEAVE('-');
		return ;
	}
	
	for( int i = 0; i < max_handles; i ++ )
	{
		tVFS_Handle	*h = &handles[i];
		LOG("handles[%i].Node = %p", i, h->Node);
		if( !h->Node )
			continue ;
		_CloseNode(h->Node);
		h->Node = NULL;
	}
	
	MM_FreeTemp(handles);
	LEAVE('-');
}

/**
 * \brief Take a backup of a set of file descriptors
 */
void *VFS_SaveHandles(int NumFDs, int *FDs)
{
	tVFS_Handle	*ret;
	const int	max_handles = *Threads_GetMaxFD(NULL);
	
	// Check if this process has any handles
	if( MM_GetPhysAddr( gaUserHandles ) == 0 )
		return NULL;

	// Allocate
	ret = malloc( NumFDs * sizeof(tVFS_Handle) );
	if( !ret )
		return NULL;	

	if( NumFDs > max_handles )
		NumFDs = max_handles;

	// Take copies of the handles
	if( FDs == NULL ) {
		memcpy(ret, gaUserHandles, NumFDs * sizeof(tVFS_Handle));
	}
	else
	{
		for( int i = 0; i < NumFDs; i ++ )
		{
			if( FDs[i] < -1 )
			{
				Log_Warning("VFS", "VFS_SaveHandles - Slot %i error FD (%i<0), ignorning",
					i, FDs[i]);
				memset(&ret[i], 0, sizeof(tVFS_Handle));
				continue ;
			}
			
			int fd = FDs[i] & (VFS_KERNEL_FLAG - 1);
			tVFS_Handle *h = VFS_GetHandle(fd);
			if(!h) {
				Log_Warning("VFS", "VFS_SaveHandles - Invalid FD 0x%x (%i) in slot %i",
					fd, FDs[i], i );
				free(ret);
				return NULL;
			}
//			Log("%i: Duplicate FD %i (%p)", i, fd, h->Node);
			memcpy( &ret[i], h, sizeof(tVFS_Handle) );
		}
	}
	
	// Reference nodes/mounts
	for( int i = 0; i < NumFDs; i ++ )
	{
		tVFS_Handle *h = &ret[i];
		// Reference node
		if( !h->Node )
			continue ;
//		Debug("VFS_SaveHandles: %i %p", i, h->Node);
		_ReferenceNode(h->Node);
		h->Mount->OpenHandleCount ++;
	}	

	return ret;
}

void VFS_RestoreHandles(int NumFDs, void *Handles)
{
	tVFS_Handle	*handles = Handles;
	const int	max_handles = *Threads_GetMaxFD(NULL);

	// NULL = nothing to do
	if( !Handles )
		return ;
	
	if( NumFDs > max_handles ) {
		Log_Notice("VFS", "RestoreHandles: Capping from %i FDs to %i", NumFDs, max_handles);
		NumFDs = max_handles;
	}

	// Allocate user handle area (and dereference existing handles)
	for( int i = 0; i < NumFDs; i ++ )
	{
		tVFS_Handle *h = &gaUserHandles[i];
		
		if( !MM_GetPhysAddr(h) )
		{
			void	*pg = (void*)( (tVAddr)h & ~(PAGE_SIZE-1) );
			if( !MM_Allocate( pg ) ) 
			{
				// OOM?
				return ;
			}
			memset(pg, 0, PAGE_SIZE);
		}
		// Safe to dereference, as Threads_CloneTCB references handles
		#if 1
		else
		{
			if(h->Node)
			{
				_CloseNode(h->Node);
				h->Mount->OpenHandleCount --;
			}
		}
		#endif
	}
	
	// Clean up existing
	// Restore handles
	memcpy(	gaUserHandles, handles, NumFDs * sizeof(tVFS_Handle) );
	// Reference when copied
	for( int i = 0; i < NumFDs; i ++ )
	{
		tVFS_Handle	*h = &gaUserHandles[i];
	
		if( !h->Node )
			continue ;
//		Debug("VFS_RestoreHandles: %i %p", i, h->Node);
		_ReferenceNode(h->Node);
		h->Mount->OpenHandleCount ++;
	}
	for( int i = NumFDs; i < max_handles; i ++ )
	{
		gaUserHandles[i].Node = NULL;
	}
}

void VFS_FreeSavedHandles(int NumFDs, void *Handles)
{
	tVFS_Handle	*handles = Handles;
	 int	i;

	// NULL = nothing to do
	if( !Handles )
		return ;	
	
	// Dereference all saved nodes
	for( i = 0; i < NumFDs; i ++ )
	{
		tVFS_Handle	*h = &handles[i];
	
		if( !h->Node )
			continue ;
		_CloseNode(h->Node);
		
		ASSERT(h->Mount->OpenHandleCount > 0);
		LOG("dec. mntpt '%s' to %i", h->Mount->MountPoint, h->Mount->OpenHandleCount-1);
		h->Mount->OpenHandleCount --;
	}
	free( Handles );
}
