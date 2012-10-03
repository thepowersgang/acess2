/*
 * Acess2 VFS
 * - By thePowersGang (John Hodge)
 * 
 * select.c
 * - Implements the select() system call (and supporting code)
 *
 * TODO: Implment timeouts (via an alarm event?)
 * TODO: Remove malloc for read/write queues
 */
#define DEBUG	0
#include <acess.h>
#include "vfs.h"
#include "vfs_int.h"
#include "vfs_ext.h"
#include <semaphore.h>
#include <threads.h>
#include <events.h>
#include <timers.h>

// === CONSTANTS ===
#define	NUM_THREADS_PER_ALLOC	4

// === TYPES ===
typedef struct sVFS_SelectListEnt	tVFS_SelectListEnt;

// === STRUCTURES ===
struct sVFS_SelectListEnt
{
	tVFS_SelectListEnt	*Next;
	tThread	*Threads[NUM_THREADS_PER_ALLOC];
};

// NOTE: Typedef is in vfs.h
struct sVFS_SelectList
{
	tMutex	Lock;
	tVFS_SelectListEnt	FirstEnt;
};

// === PROTOTYPES ===
// int	VFS_SelectNode(tVFS_Node *Node, enum eVFS_SelectTypes Type, tTime *Timeout);
// int	VFS_Select(int MaxHandle, fd_set *ReadHandles, fd_set *WriteHandles, fd_set *ErrHandles, tTime *Timeout, Uint32 ExtraEvents, BOOL IsKernel);
// int	VFS_MarkFull(tVFS_Node *Node, BOOL IsBufferFull);
// int	VFS_MarkAvaliable(tVFS_Node *Node, BOOL IsDataAvaliable);
// int	VFS_MarkError(tVFS_Node *Node, BOOL IsErrorState);
 int	VFS_int_Select_GetType(int Type, tVFS_Node *Node, tVFS_SelectList ***List, int **Flag, int *WantedFlag, int *MaxAllowed);
 int	VFS_int_Select_Register(tThread *Thread, int MaxHandle, fd_set *Handles, int Type, BOOL IsKernel);
 int	VFS_int_Select_Deregister(tThread *Thread, int MaxHandle, fd_set *Handles, int Type, BOOL IsKernel);
 int	VFS_int_Select_AddThread(tVFS_SelectList *List, tThread *Thread, int MaxAllowed);
void	VFS_int_Select_RemThread(tVFS_SelectList *List, tThread *Thread);
void	VFS_int_Select_SignalAll(tVFS_SelectList *List);

// === GLOBALS ===

// === FUNCTIONS ===
int VFS_SelectNode(tVFS_Node *Node, int TypeFlags, tTime *Timeout, const char *Name)
{
	tThread	*thisthread = Proc_GetCurThread();
	 int	ret, type;
	
	ENTER("pNode iTypeFlags pTimeout sName", Node, TypeFlags, Timeout, Name);
	
	// Initialise
	for( type = 0; type < 3; type ++ )
	{
		tVFS_SelectList	**list;
		 int	*flag, wanted, maxAllowed;
		if( !(TypeFlags & (1 << type)) )	continue;
		if( VFS_int_Select_GetType(type, Node, &list, &flag, &wanted, &maxAllowed) ) {
			LEAVE('i', -1);
			return -1;
		}
	
		// Alloc if needed
		if( !*list )	*list = calloc(1, sizeof(tVFS_SelectList));
	
		VFS_int_Select_AddThread(*list, thisthread, maxAllowed);
		if( *flag == wanted )
		{
			VFS_int_Select_RemThread(*list, thisthread);
			LEAVE('i', 1);
			return 1;
		}
	}

	// Wait for things	
	if( !Timeout )
	{
		LOG("Semaphore_Wait()");
		// TODO: Actual timeout
		Threads_WaitEvents( THREAD_EVENT_VFS );
	}
	else if( *Timeout > 0 )
	{
		tTimer *t = Time_AllocateTimer(NULL, NULL);
		// Clear timer event
		Threads_ClearEvent( THREAD_EVENT_TIMER );
		// TODO: Convert *Timeout?
		LOG("Timeout %lli ms", *Timeout);
		Time_ScheduleTimer( t, *Timeout );
		// Wait for the timer or a VFS event
		Threads_WaitEvents( THREAD_EVENT_VFS|THREAD_EVENT_TIMER );
		Time_FreeTimer(t);
	}
	
	// Get return value
	ret = 0;
	for( type = 0; type < 3; type ++ )
	{
		tVFS_SelectList	**list;
		 int	*flag, wanted, maxAllowed;
		if( !(TypeFlags & (1 << type)) )	continue;
		VFS_int_Select_GetType(type, Node, &list, &flag, &wanted, &maxAllowed);
		LOG("VFS_int_Select_RemThread()");
		VFS_int_Select_RemThread(*list, thisthread);
		ret = ret || *flag == wanted;
	}

	Threads_ClearEvent( THREAD_EVENT_VFS );
	Threads_ClearEvent( THREAD_EVENT_TIMER );
	
	LEAVE('i', ret);
	return ret;
}

int VFS_Select(int MaxHandle, fd_set *ReadHandles, fd_set *WriteHandles, fd_set *ErrHandles, tTime *Timeout, Uint32 ExtraEvents, BOOL IsKernel)
{
	tThread	*thisthread = Proc_GetCurThread();
	 int	ret;
	
	ENTER("iMaxHandle pReadHandles pWriteHandles pErrHandles pTimeout bIsKernel",
		MaxHandle, ReadHandles, WriteHandles, ErrHandles, Timeout, IsKernel);
	
	// Notes: The idea is to make sure we only enter wait (Threads_WaitEvents)
	// if we are going to be woken up (either by an event at a later time,
	// or by an event that happened while or before we were registering).
	// Hence, register must happen _before_ we check the state flag
	// (See VFS_int_Select_Register), that way either we pick up the flag,
	// or the semaphore is incremeneted (or both, but never none)
	
	// Register with nodes
	ret  = VFS_int_Select_Register(thisthread, MaxHandle, ReadHandles, 0, IsKernel);
	ret += VFS_int_Select_Register(thisthread, MaxHandle, WriteHandles, 1, IsKernel);
	ret += VFS_int_Select_Register(thisthread, MaxHandle, ErrHandles, 2, IsKernel);
	
	LOG("Register ret = %i", ret);
	
	// If there were events waiting, de-register and return
	if( ret > 0 )
	{
		ret  = VFS_int_Select_Deregister(thisthread, MaxHandle, ReadHandles, 0, IsKernel);
		ret += VFS_int_Select_Deregister(thisthread, MaxHandle, WriteHandles, 1, IsKernel);
		ret += VFS_int_Select_Deregister(thisthread, MaxHandle, ErrHandles, 2, IsKernel);
		LEAVE('i', ret);
		return ret;
	}

	// Wait for things	
	if( !Timeout )
	{
		LOG("Semaphore_Wait()");
		// TODO: Actual timeout
		Threads_WaitEvents( THREAD_EVENT_VFS|ExtraEvents );
	}
	else if( *Timeout > 0 )
	{
		tTimer *t = Time_AllocateTimer(NULL, NULL);
		// Clear timer event
		Threads_ClearEvent( THREAD_EVENT_TIMER );
		// TODO: Convert *Timeout?
		LOG("Timeout %lli ms", *Timeout);
		Time_ScheduleTimer( t, *Timeout );
		// Wait for the timer or a VFS event
		Threads_WaitEvents( THREAD_EVENT_VFS|THREAD_EVENT_TIMER|ExtraEvents );
		Time_FreeTimer(t);
	}
	// Fill output (modify *Handles)
	// - Also, de-register
	ret  = VFS_int_Select_Deregister(thisthread, MaxHandle, ReadHandles, 0, IsKernel);
	ret += VFS_int_Select_Deregister(thisthread, MaxHandle, WriteHandles, 1, IsKernel);
	ret += VFS_int_Select_Deregister(thisthread, MaxHandle, ErrHandles, 2, IsKernel);
	
	Threads_ClearEvent( THREAD_EVENT_VFS );
	Threads_ClearEvent( THREAD_EVENT_TIMER );
	
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
	ENTER("pNode bIsBufferFull", Node, IsBufferFull);
	Node->BufferFull = !!IsBufferFull;
	if( !Node->BufferFull )
		VFS_int_Select_SignalAll(Node->WriteThreads);
	LEAVE('i', 0);
	return 0;
}

// Mark a node as errored
int VFS_MarkError(tVFS_Node *Node, BOOL IsErrorState)
{
	ENTER("pNode bIsErrorState", Node, IsErrorState);
	Node->ErrorOccurred = !!IsErrorState;
	if( Node->ErrorOccurred )
		VFS_int_Select_SignalAll(Node->ErrorThreads);
	LEAVE('i', 0);
	return 0;
}

// --- Internal ---
int VFS_int_Select_GetType(int Type, tVFS_Node *Node, tVFS_SelectList ***List, int **Flag, int *WantedFlag, int *MaxAllowed)
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
int VFS_int_Select_Register(tThread *Thread, int MaxHandle, fd_set *Handles, int Type, BOOL IsKernel)
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
int VFS_int_Select_Deregister(tThread *Thread, int MaxHandle, fd_set *Handles, int Type, BOOL IsKernel)
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
int VFS_int_Select_AddThread(tVFS_SelectList *List, tThread *Thread, int MaxAllowed)
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
				Mutex_Release(&List->Lock);
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

void VFS_int_Select_RemThread(tVFS_SelectList *List, tThread *Thread)
{
	 int	i;
	tVFS_SelectListEnt	*block, *prev = NULL;
	
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
			if( block->Threads[i]  )
			{
				LOG("block(%p)->Threads[%i] = %p", block, i, block->Threads[i]);
				Threads_PostEvent( block->Threads[i], THREAD_EVENT_VFS );
			}
		}
		
		block = block->Next;
	} while(block);
	
	Mutex_Release(&List->Lock);
	
	LEAVE('-');
}
