/*
 * Acess2 VFS
 * - By thePowersGang (John Hodge)
 * 
 * select.c
 * - Implements the select() system call (and supporting code)
 */
#define DEBUG	0
#include <acess.h>
#include "vfs.h"
#include "vfs_int.h"
#include "vfs_ext.h"
#include <semaphore.h>

// === CONSTANTS ===
#define	NUM_THREADS_PER_ALLOC	4

// === TYPES ===
typedef struct sVFS_SelectThread	tVFS_SelectThread;
typedef struct sVFS_SelectListEnt	tVFS_SelectListEnt;

// === STRUCTURES ===
struct sVFS_SelectListEnt
{
	tVFS_SelectListEnt	*Next;
	tVFS_SelectThread	*Threads[NUM_THREADS_PER_ALLOC];
};

struct sVFS_SelectList
{
	tMutex	Lock;
	tVFS_SelectListEnt	FirstEnt;
};

struct sVFS_SelectThread
{
	//! \brief Marks the thread as actively using this select
	 int	IsActive;
	//! \brief Semaphore to atomically put the listener to sleep
	tSemaphore	SleepHandle;	// TODO: Allow timeouts (by setting an alarm?)
};

// === PROTOTYPES ===
 int	VFS_Select(int MaxHandle, fd_set *ReadHandles, fd_set *WriteHandles, fd_set *ErrHandles, tTime *Timeout, int IsKernel);
 int	VFS_MarkFull(tVFS_Node *Node, BOOL IsBufferFull);
 int	VFS_MarkAvaliable(tVFS_Node *Node, BOOL IsDataAvaliable);
 int	VFS_MarkError(tVFS_Node *Node, BOOL IsErrorState);
 int	VFS_int_Select_Register(tVFS_SelectThread *Thread, int MaxHandle, fd_set *Handles, int Type, BOOL IsKernel);
 int	VFS_int_Select_Deregister(tVFS_SelectThread *Thread, int MaxHandle, fd_set *Handles, int Type, BOOL IsKernel);
 int	VFS_int_Select_AddThread(tVFS_SelectList *List, tVFS_SelectThread *Thread, int MaxAllowed);
void	VFS_int_Select_RemThread(tVFS_SelectList *List, tVFS_SelectThread *Thread);

// === GLOBALS ===

// === FUNCTIONS ===
int VFS_Select(int MaxHandle, fd_set *ReadHandles, fd_set *WriteHandles, fd_set *ErrHandles, tTime *Timeout, int IsKernel)
{
	tVFS_SelectThread	thread_info;
	 int	ret;
	
	// Register with nodes
	ret  = VFS_int_Select_Register(&thread_info, MaxHandle, ReadHandles, 0, IsKernel);
	ret += VFS_int_Select_Register(&thread_info, MaxHandle, WriteHandles, 1, IsKernel);
	ret += VFS_int_Select_Register(&thread_info, MaxHandle, ErrHandles, 2, IsKernel);
	
	// If there were events waiting, de-register and return
	if( ret )
	{
		ret = VFS_int_Select_Deregister(&thread_info, MaxHandle, ReadHandles, 0, IsKernel);
		ret += VFS_int_Select_Deregister(&thread_info, MaxHandle, ReadHandles, 1, IsKernel);
		ret += VFS_int_Select_Deregister(&thread_info, MaxHandle, ReadHandles, 2, IsKernel);
		return ret;
	}
	
	// TODO: Implement timeout
	
	// Wait (only if there is no timeout, or it is greater than zero
	if( !Timeout || *Timeout > 0 )
	{
		ret = Semaphore_Wait(&thread_info.SleepHandle, 0);
	}
	
	// Fill output (modify *Handles)
	// - Also, de-register
	ret = VFS_int_Select_Deregister(&thread_info, MaxHandle, ReadHandles, 0, IsKernel);
	ret += VFS_int_Select_Deregister(&thread_info, MaxHandle, ReadHandles, 1, IsKernel);
	ret += VFS_int_Select_Deregister(&thread_info, MaxHandle, ReadHandles, 2, IsKernel);
	return ret;
}

// --- Internal ---
/**
 * \return Number of files with an action
 */
int VFS_int_Select_Register(tVFS_SelectThread *Thread, int MaxHandle, fd_set *Handles, int Type, BOOL IsKernel)
{
	 int	i, numFlagged = 0;
	tVFS_SelectList	**list;
	 int	*flag, wantedFlagValue;
	 int	maxAllowed;
	
	if( !Handles )	return 0;
	
	for( i = 0; i < MaxHandle; i ++ )
	{
		tVFS_Handle	*handle;
		
		// Is the descriptor set
		if( !FD_ISSET(i, Handles) )	continue;
		
		handle = VFS_GetHandle( i | (IsKernel?VFS_KERNEL_FLAG:0) );
		// Is the handle valid?
		if( !handle || !handle->Node )
		{
			if( Type == 2 ) {	// Bad FD counts as an error
				numFlagged ++;
			}
			else {
				FD_CLR(i, Handles);
			}
			continue;
		}
	
		// Get the type of the listen
		switch(Type)
		{
		case 0:	// Read
			list = &handle->Node->ReadThreads;
			flag = &handle->Node->DataAvaliable;
			wantedFlagValue = 1;
			maxAllowed = 1;	// Max of 1 for read
			break;
		case 1:	// Write
			list = &handle->Node->WriteThreads;
			flag = &handle->Node->BufferFull;
			wantedFlagValue = 0;
			maxAllowed = 1;	// Max of 1 for write
			break;
		case 2:	// Error
			list = &handle->Node->ErrorThreads;
			flag = &handle->Node->ErrorOccurred;
			wantedFlagValue = 1;
			maxAllowed = -1;	// No max for error listeners
			break;
		default:
			Log_Error("VFS", "VFS_int_Select_Deregister: BUG CHECK, Unknown Type %i", Type);
			return 0;
		}
		
		// Alloc if needed
		if( !*list ) {
			*list = calloc(1, sizeof(tVFS_SelectList));
		}
		
		// Register
		if( VFS_int_Select_AddThread(*list, Thread, maxAllowed ) )
		{
			// Oops, error (or just no space)
			FD_CLR(i, Handles);
		}
		
		// Check for the flag
		if( !!*flag == !!wantedFlagValue )
			numFlagged ++;
	}
	
	return numFlagged;
}
/**
 * \return Number of files with an action
 */
int VFS_int_Select_Deregister(tVFS_SelectThread *Thread, int MaxHandle, fd_set *Handles, int Type, BOOL IsKernel)
{
	 int	i, numFlagged = 0;
	tVFS_SelectList	**list;
	 int	*flag, wantedFlagValue;
	
	if( !Handles )	return 0;
	
	for( i = 0; i < MaxHandle; i ++ )
	{
		tVFS_Handle	*handle;
		
		// Is the descriptor set
		if( !FD_ISSET(i, Handles) )	continue;
		
		handle = VFS_GetHandle( i | (IsKernel?VFS_KERNEL_FLAG:0) );
		// Is the handle valid?
		if( !handle || !handle->Node )
		{
			if( Type == 2 ) {	// Bad FD counts as an error
				numFlagged ++;
			}
			else {
				FD_CLR(i, Handles);
			}
			continue;
		}
	
		// Get the type of the listen
		switch(Type)
		{
		case 0:	// Read
			list = &handle->Node->ReadThreads;
			flag = &handle->Node->DataAvaliable;
			wantedFlagValue = 1;
			break;
		case 1:	// Write
			list = &handle->Node->WriteThreads;
			flag = &handle->Node->BufferFull;
			wantedFlagValue = 0;
			break;
		case 2:	// Error
			list = &handle->Node->ErrorThreads;
			flag = &handle->Node->ErrorOccurred;
			wantedFlagValue = 1;
			break;
		default:
			Log_Error("VFS", "VFS_int_Select_Deregister: BUG CHECK, Unknown Type %i", Type);
			return 0;
		}
		
		// Remove
		VFS_int_Select_RemThread(*list, Thread );
		
		// Check for the flag
		if( !!*flag == !!wantedFlagValue ) {
			numFlagged ++;
		}
		else {
			FD_CLR(i, Handles);
		}
	}
	
	return numFlagged;
}

/**
 * \return Boolean failure
 */
int VFS_int_Select_AddThread(tVFS_SelectList *List, tVFS_SelectThread *Thread, int MaxAllowed)
{
	 int	i, count = 0;
	tVFS_SelectListEnt	*block, *prev;
	
	// Lock to avoid concurrency issues
	Mutex_Acquire(&List->Lock);
	
	block = &List->FirstEnt;
	
	// Look for free space
	do
	{
		for( i = 0; i < NUM_THREADS_PER_ALLOC; i ++ )
		{
			if( block->Threads[i] == NULL )
			{
				block->Threads[i] = Thread;
				Mutex_Release(&List->Lock);
				return 0;
			}
			count ++;
			if( MaxAllowed && count >= MaxAllowed ) {
				return 1;
			}
		}
		
		prev = block;
		block = block->Next;
	} while(block);
	
	// Create new block
	block = malloc( sizeof(tVFS_SelectListEnt) );
	if( !block ) {
		Log_Warning("VFS", "VFS_int_Select_AddThread: malloc() failed");
		Mutex_Release(&List->Lock);
		return -1;
	}
	block->Next = NULL;
	block->Threads[0] = Thread;
	for( i = 1; i < NUM_THREADS_PER_ALLOC; i ++ )
	{
		block->Threads[i] = NULL;
	}
	
	// Add to list
	prev->Next = block;
	
	// Release
	Mutex_Release(&List->Lock);
	
	return 0;
}

void VFS_int_Select_RemThread(tVFS_SelectList *List, tVFS_SelectThread *Thread)
{
	 int	i;
	tVFS_SelectListEnt	*block, *prev;
	
	// Lock to avoid concurrency issues
	Mutex_Acquire(&List->Lock);
	
	block = &List->FirstEnt;
	
	// Look for the thread
	do
	{
		for( i = 0; i < NUM_THREADS_PER_ALLOC; i ++ )
		{
			if( block->Threads[i] == Thread )
			{
				block->Threads[i] = NULL;
				
				// Check if this block is empty
				if( block != &List->FirstEnt )
				{
					for( i = 0; i < NUM_THREADS_PER_ALLOC; i ++ )
						if( block->Threads[i] )
							break;
					// If empty, free it
					if( i == NUM_THREADS_PER_ALLOC ) {
						prev->Next = block->Next;
						free(block);
					}
					//TODO: If not empty, check if it can be merged downwards
				}
				
				Mutex_Release(&List->Lock);
				return ;
			}
		}
		
		prev = block;
		block = block->Next;
	} while(block);
	
	// Not on list, is this an error?
}

