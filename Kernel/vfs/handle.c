/*
 * Acess2 VFS
 * - AllocHandle, GetHandle
 */
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
#if 0
tVFS_Handle	*VFS_GetHandle(int FD);
#endif
 int	VFS_AllocHandle(int FD, tVFS_Node *Node, int Mode);

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
		if(FD >= *Threads_GetMaxFD())	return NULL;
		h = &gaUserHandles[ FD ];
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
		 int	max_handles = *Threads_GetMaxFD();
		// Allocate Buffer
		if( MM_GetPhysAddr( (tVAddr)gaUserHandles ) == 0 )
		{
			Uint	addr, size;
			size = max_handles * sizeof(tVFS_Handle);
			for(addr = 0; addr < size; addr += 0x1000)
			{
				if( !MM_Allocate( (tVAddr)gaUserHandles + addr ) )
				{
					Warning("OOM - VFS_AllocHandle");
					Threads_Exit(0, 0xFF);	// Terminate user
				}
			}
			memset( gaUserHandles, 0, size );
		}
		// Get a handle
		for( i = 0; i < max_handles; i ++ )
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
		if( MM_GetPhysAddr( (tVAddr)gaKernelHandles ) == 0 )
		{
			Uint	addr, size;
			size = MAX_KERNEL_FILES * sizeof(tVFS_Handle);
			for(addr = 0; addr < size; addr += 0x1000)
			{
				if( !MM_Allocate( (tVAddr)gaKernelHandles + addr ) )
				{
					Panic("OOM - VFS_AllocHandle");
					Threads_Exit(0, 0xFF);	// Terminate application (get some space back)
				}
			}
			memset( gaKernelHandles, 0, size );
		}
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

void VFS_ReferenceUserHandles(void)
{
	 int	i;
	 int	max_handles = *Threads_GetMaxFD();
	
	for( i = 0; i < max_handles; i ++ )
	{
		tVFS_Handle	*h;
		h = &gaUserHandles[i];
		if( h->Node->Type && h->Node->Type->Reference )
			h->Node->Type->Reference( h->Node );
	}
}

void VFS_CloseAllUserHandles(void)
{
	 int	i;
	 int	max_handles = *Threads_GetMaxFD();
	
	for( i = 0; i < max_handles; i ++ )
	{
		tVFS_Handle	*h;
		h = &gaUserHandles[i];
		if( h->Node->Type && h->Node->Type->Close )
			h->Node->Type->Close( h->Node );
	}
}

/**
 * \brief Take a backup of a set of file descriptors
 */
void *VFS_SaveHandles(int NumFDs, int *FDs)
{
	tVFS_Handle	*ret;
	 int	i;
	
	// Check if this process has any handles
	if( MM_GetPhysAddr( (tVAddr)gaUserHandles ) == 0 )
		return NULL;

	// Allocate
	ret = malloc( NumFDs * sizeof(tVFS_Handle) );
	if( !ret )
		return NULL;	

	// Take copies of the handles
	for( i = 0; i < NumFDs; i ++ )
	{
		tVFS_Handle	*h;
		h = VFS_GetHandle(FDs[i] & (VFS_KERNEL_FLAG - 1));
		if(!h) {
			Log_Warning("VFS", "VFS_SaveHandles - Invalid FD %i",
				FDs[i] & (VFS_KERNEL_FLAG - 1) );
			free(ret);
			return NULL;
		}
		memcpy( &ret[i], h, sizeof(tVFS_Handle) );
		
		// Reference node
		if( h->Node->Type && h->Node->Type->Reference )
			h->Node->Type->Reference( h->Node );
	}	

	return ret;
}

void VFS_RestoreHandles(int NumFDs, void *Handles)
{
	tVFS_Handle	*handles = Handles;
	 int	i;

	// NULL = nothing to do
	if( !Handles )
		return ;	

	// Check if there is already a set of handles
	if( MM_GetPhysAddr( (tVAddr)gaUserHandles ) != 0 )
		return ;
	
	
	// Allocate user handle area
	{
		Uint	addr, size;
		 int	max_handles = *Threads_GetMaxFD();
		size = max_handles * sizeof(tVFS_Handle);
		for(addr = 0; addr < size; addr += 0x1000)
		{
			if( !MM_Allocate( (tVAddr)gaUserHandles + addr ) )
			{
				Warning("OOM - VFS_AllocHandle");
				Threads_Exit(0, 0xFF);	// Terminate user
			}
		}
		memset( gaUserHandles, 0, size );
	}
	
	// Restore handles
	memcpy(	gaUserHandles, handles, NumFDs * sizeof(tVFS_Handle) );
	// Reference when copied
	for( i = 0; i < NumFDs; i ++ )
	{
		tVFS_Handle	*h = &handles[i];
	
		if( h->Node->Type && h->Node->Type->Reference )
			h->Node->Type->Reference( h->Node );
	}
}

void VFS_FreeSavedHandles(int NumFDs, void *Handles)
{
	tVFS_Handle	*handles = Handles;
	 int	i;
	
	// Dereference all saved nodes
	for( i = 0; i < NumFDs; i ++ )
	{
		tVFS_Handle	*h = &handles[i];
	
		if( h->Node->Type && h->Node->Type->Close )
			h->Node->Type->Close( h->Node );
	}
	free( Handles );
}
