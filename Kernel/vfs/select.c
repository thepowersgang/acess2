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
	//! \brief Semaphore to atomically put the listener to sleep
	tSemaphore	SleepHandle;	// TODO: Allow timeouts (by setting an alarm?)
};

// === PROTOTYPES ===
// int	VFS_SelectNode(tVFS_Node *Node, enum eVFS_SelectTypes Type, tTime *Timeout);
// int	VFS_Select(int MaxHandle, fd_set *ReadHandles, fd_set *WriteHandles, fd_set *ErrHandles, tTime *Timeout, BOOL IsKernel);
// int	VFS_MarkFull(tVFS_Node *Node, BOOL IsBufferFull);
// int	VFS_MarkAvaliable(tVFS_Node *Node, BOOL IsDataAvaliable);
// int	VFS_MarkError(tVFS_Node *Node, BOOL IsErrorState);
 int	VFS_int_Select_GetType(enum eVFS_SelectTypes Type, tVFS_Node *Node, tVFS_SelectList ***List, int **Flag, int *WantedFlag, int *MaxAllowed);
 int	VFS_int_Select_Register(tVFS_SelectThread *Thread, int MaxHandle, fd_set *Handles, enum eVFS_SelectTypes Type, BOOL IsKernel);
 int	VFS_int_Select_Deregister(tVFS_SelectThread *Thread, int MaxHandle, fd_set *Handles, enum eVFS_SelectTypes Type, BOOL IsKernel);
 int	VFS_int_Select_AddThread(tVFS_SelectList *List, tVFS_SelectThread *Thread, int MaxAllowed);
void	VFS_int_Select_RemThread(tVFS_SelectList *List, tVFS_SelectThread *Thread);
void	VFS_int_Select_SignalAll(tVFS_SelectList *List);

// === GLOBALS ===

// === FUNCTIONS ===
int VFS_SelectNode(tVFS_Node *Node, enum eVFS_SelectTypes Type, tTime *Timeout, const char *Name)
{
	tVFS_SelectThread	*thread_info;
	tVFS_SelectList	**list;
	 int	*flag, wanted, maxAllowed;
	
	ENTER("pNode iType pTimeout", Node, Type, Timeout);
	
	if( VFS_int_Select_GetType(Type, Node, &list, &flag, &wanted, &maxAllowed) ) {
		LEAVE('i', -1);
		return -1;
	}
	
	thread_info = malloc(sizeof(tVFS_SelectThread));
	if(!thread_info)	return -1;
	
	Semaphore_Init(&thread_info->SleepHandle, 0, 0, "VFS_SelectNode()", Name);
	
	LOG("list=%p, flag=%p, wanted=%i, maxAllowed=%i", list, flag, wanted, maxAllowed);
	
	// Alloc if needed
	if( !*list ) {
		*list = calloc(1, sizeof(tVFS_SelectList));
	}
	
	VFS_int_Select_AddThread(*list, thread_info, maxAllowed);
	if( *flag == wanted )
	{
		VFS_int_Select_RemThread(*list, thread_info);
		free(thread_info);
		LEAVE('i', 1);
		return 1;
	}
	
	if( !Timeout || *Timeout > 0 )
	{
		LOG("Semaphore_Wait()");
		// TODO: Actual timeout
		Semaphore_Wait(&thread_info->SleepHandle, 1);
	}
	
	LOG("VFS_int_Select_RemThread()");
	VFS_int_Select_RemThread(*list, thread_info);
	
	free(thread_info);
	
	LEAVE('i', *flag == wanted);
	return *flag == wanted;
}

int VFS_Select(int MaxHandle, fd_set *ReadHandles, fd_set *WriteHandles, fd_set *ErrHandles, tTime *Timeout, BOOL IsKernel)
{
	tVFS_SelectThread	*thread_info;
	 int	ret;
	
	thread_info = malloc(sizeof(tVFS_SelectThread));
	if(!thread_info)	return -1;
	
	ENTER("iMaxHandle pReadHandles pWriteHandles pErrHandles pTimeout bIsKernel",
		MaxHandle, ReadHandles, WriteHandles, ErrHandles, Timeout, IsKernel);
	
	Semaphore_Init(&thread_info->SleepHandle, 0, 0, "VFS_Select()", "");
	
	// Notes: The idea is to make sure we only enter wait (on the semaphore)
	// if we are going to be woken up (either by an event at a later time,
	// or by an event that happened while or before we were registering).
	// Hence, register must happen _before_ we check the state flag
	// (See VFS_int_Select_Register), that way either we pick up the flag,
	// or the semaphore is incremeneted (or both, but never none)
	
	// Register with nodes
	ret  = VFS_int_Select_Register(thread_info, MaxHandle, ReadHandles, 0, IsKernel);
	ret += VFS_int_Select_Register(thread_info, MaxHandle, WriteHandles, 1, IsKernel);
	ret += VFS_int_Select_Register(thread_info, MaxHandle, ErrHandles, 2, IsKernel);
	
	LOG("Register ret = %i", ret);
	
	// If there were events waiting, de-register and return
	if( ret )
	{
		ret  = VFS_int_Select_Deregister(thread_info, MaxHandle, ReadHandles, 0, IsKernel);
		ret += VFS_int_Select_Deregister(thread_info, MaxHandle, WriteHandles, 1, IsKernel);
		ret += VFS_int_Select_Deregister(thread_info, MaxHandle, ErrHandles, 2, IsKernel);
		free(thread_info);
		LEAVE('i', ret);
		return ret;
	}
	
	// TODO: Implement timeout
	
	// Wait (only if there is no timeout, or it is greater than zero
	if( !Timeout || *Timeout > 0 )
	{
		ret = Semaphore_Wait(&thread_info->SleepHandle, 1);
		// TODO: Do something with ret
	}
	
	// Fill output (modify *Handles)
	// - Also, de-register
	ret  = VFS_int_Select_Deregister(thread_info, MaxHandle, ReadHandles, 0, IsKernel);
	ret += VFS_int_Select_Deregister(thread_info, MaxHandle, WriteHandles, 1, IsKernel);
	ret += VFS_int_Select_Deregister(thread_info, MaxHandle, ErrHandles, 2, IsKernel);
	free(thread_info);
	LEAVE('i', ret);
	return ret;
}

// Mark a node as having data ready for reading
int VFS_MarkAvaliable(tVFS_Node *Node, BOOL IsDataAvaliable)
{
	ENTER("pNode bIsDataAvaliable", Node, IsDataAvaliable);
	Node->DataAvaliable = !!IsDataAvaliable;
	if( Node->DataAvaliable )
		VFS_int_Select_SignalAll(Node->ReadThreads);
	LEAVE('i', 0);
	return 0;
}

// Mark a node as having a full buffer
int VFS_MarkFull(tVFS_Node *Node, BOOL IsBufferFull)
{
	ENTER("pNode bIsDataAvaliable", Node, IsBufferFull);
	Node->BufferFull = !!IsBufferFull;
	if( !Node->BufferFull )
		VFS_int_Select_SignalAll(Node->WriteThreads);
	LEAVE('i', 0);
	return 0;
}

// Mark a node as errored
int VFS_MarkError(tVFS_Node *Node, BOOL IsErrorState)
{
	ENTER("pNode bIsDataAvaliable", Node, IsErrorState);
	Node->ErrorOccurred = !!IsErrorState;
	if( Node->ErrorOccurred )
		VFS_int_Select_SignalAll(Node->ErrorThreads);
	LEAVE('i', 0);
	return 0;
}

// --- Internal ---
int VFS_int_Select_GetType(enum eVFS_SelectTypes Type, tVFS_Node *Node, tVFS_SelectList ***List, int **Flag, int *WantedFlag, int *MaxAllowed)
{
	// Get the type of the listen
	switch(Type)
	{
	case 0:	// Read
		if(List)	*List = &Node->ReadThreads;
		if(Flag)	*Flag = &Node->DataAvaliable;
		if(WantedFlag)	*WantedFlag = 1;
		if(MaxAllowed)	*MaxAllowed = 1;	// Max of 1 for read
		break;
	case 1:	// Write
		if(List)	*List = &Node->WriteThreads;
		if(Flag)	*Flag = &Node->BufferFull;
		if(WantedFlag)	*WantedFlag = 0;
		if(MaxAllowed)	*MaxAllowed = 1;	// Max of 1 for write
		break;
	case 2:	// Error
		if(List)	*List = &Node->ErrorThreads;
		if(Flag)	*Flag = &Node->ErrorOccurred;
		if(WantedFlag)	*WantedFlag = 1;
		if(MaxAllowed)	*MaxAllowed = -1;	// No max for error listeners
		break;
	default:
		Log_Error("VFS", "VFS_int_Select_GetType: BUG CHECK, Unknown Type %i", Type);
		return 1;
	}
	return 0;
}

/**
 * \return Number of files with an action
 */
int VFS_int_Select_Register(tVFS_SelectThread *Thread, int MaxHandle, fd_set *Handles, enum eVFS_SelectTypes Type, BOOL IsKernel)
{
	 int	i, numFlagged = 0;
	tVFS_SelectList	**list;
	 int	*flag, wantedFlagValue;
	 int	maxAllowed;
	
	if( !Handles )	return 0;
	
	ENTER("pThread iMaxHandle pHandles iType iIsKernel", Thread, MaxHandle, Handles, Type, IsKernel);
	
	for( i = 0; i < MaxHandle; i ++ )
	{
		tVFS_Handle	*handle;
		
		// Is the descriptor set
		if( !FD_ISSET(i, Handles) )	continue;
		LOG("FD #%i", i);
		
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
		if( VFS_int_Select_GetType(Type, handle->Node, &list, &flag, &wantedFlagValue, &maxAllowed) ) {
			LEAVE('i', 0);
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
	
	LEAVE('i', numFlagged);
	
	return numFlagged;
}
/**
 * \return Number of files with an action
 */
int VFS_int_Select_Deregister(tVFS_SelectThread *Thread, int MaxHandle, fd_set *Handles, enum eVFS_SelectTypes Type, BOOL IsKernel)
{
	 int	i, numFlagged = 0;
	tVFS_SelectList	**list;
	 int	*flag, wantedFlagValue;
	
	if( !Handles )	return 0;
	
	ENTER("pThread iMaxHandle pHandles iType iIsKernel", Thread, MaxHandle, Handles, Type, IsKernel);
	
	for( i = 0; i < MaxHandle; i ++ )
	{
		tVFS_Handle	*handle;
		
		// Is the descriptor set
		if( !FD_ISSET(i, Handles) )	continue;
		LOG("FD #%i", i);
		
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
	
		// Get the type of the listen
		if( VFS_int_Select_GetType(Type, handle->Node, &list, &flag, &wantedFlagValue, NULL) ) {
			LEAVE('i', 0);
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
	
	LEAVE('i', numFlagged);
	
	return numFlagged;
}

/**
 * \return Boolean failure
 */
int VFS_int_Select_AddThread(tVFS_SelectList *List, tVFS_SelectThread *Thread, int MaxAllowed)
{
	 int	i, count = 0;
	tVFS_SelectListEnt	*block, *prev;
	
	ENTER("pList pThread iMaxAllowed", List, Thread, MaxAllowed);
	
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
				LEAVE('i', 0);
				return 0;
			}
			count ++;
			if( MaxAllowed && count >= MaxAllowed ) {
				LEAVE('i', 1);
				return 1;
			}
		}
		
		prev = block;
		block = block->Next;
	} while(block);
	
	LOG("New block");
	
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
	
	LEAVE('i', 0);
	return 0;
}

void VFS_int_Select_RemThread(tVFS_SelectList *List, tVFS_SelectThread *Thread)
{
	 int	i;
	tVFS_SelectListEnt	*block, *prev;
	
	ENTER("pList pThread", List, Thread);
	
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
						LOG("Deleting block");
						prev->Next = block->Next;
						free(block);
					}
					//TODO: If not empty, check if it can be merged downwards
				}
				
				Mutex_Release(&List->Lock);
				LEAVE('-');
				return ;
			}
		}
		
		prev = block;
		block = block->Next;
	} while(block);
	
	// Not on list, is this an error?
	
	Mutex_Release(&List->Lock);
	
	LOG("Not on list");
	LEAVE('-');
}

/**
 * \brief Signal all threads on a list
 */
void VFS_int_Select_SignalAll(tVFS_SelectList *List)
{
	 int	i;
	tVFS_SelectListEnt	*block;
	
	if( !List )	return ;
	
	ENTER("pList", List);
	
	// Lock to avoid concurrency issues
	Mutex_Acquire(&List->Lock);
	
	block = &List->FirstEnt;
	
	// Look for the thread
	do
	{
		for( i = 0; i < NUM_THREADS_PER_ALLOC; i ++ )
		{
			LOG("block->Threads[i] = %p", block->Threads[i]);
			if( block->Threads[i]  )
			{
				Semaphore_Signal( &block->Threads[i]->SleepHandle, 1 );
			}
		}
		
		block = block->Next;
	} while(block);
	
	Mutex_Release(&List->Lock);
	
	LEAVE('-');
}
