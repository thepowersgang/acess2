/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * messages.c
 * - IPC Messages
 */
#define DEBUG	0
#include <acess.h>
#include <threads.h>
#include <threads_int.h>
#include <errno.h>

// === IMPORTS ===
extern tShortSpinlock	glThreadListLock;

// === CODE ===
/**
 * \fn int Proc_SendMessage(Uint *Err, Uint Dest, int Length, void *Data)
 * \brief Send an IPC message
 * \param Err	Pointer to the errno variable
 * \param Dest	Destination Thread
 * \param Length	Length of the message
 * \param Data	Message data
 */
int Proc_SendMessage(Uint *Err, Uint Dest, int Length, void *Data)
{
	tThread	*thread;
	tMsg	*msg;
	
	ENTER("pErr iDest iLength pData", Err, Dest, Length, Data);
	
	if(Length <= 0 || !Data) {
		*Err = -EINVAL;
		LEAVE_RET('i', -1);
	}
	
	// Get thread
	thread = Threads_GetThread( Dest );
	
	// Error check
	if(!thread)	LEAVE_RET('i', -1);
	
	// Get Spinlock
	SHORTLOCK( &thread->IsLocked );
	
	// Check if thread is still alive
	if(thread->Status == THREAD_STAT_DEAD) {
		SHORTREL( &thread->IsLocked );
		LEAVE_RET('i', -1);
	}
	
	// Create message
	msg = malloc( sizeof(tMsg)+Length );
	msg->Next = NULL;
	msg->Source = Proc_GetCurThread()->TID;
	msg->Length = Length;
	memcpy(msg->Data, Data, Length);
	
	// If there are already messages
	if(thread->LastMessage) {
		thread->LastMessage->Next = msg;
		thread->LastMessage = msg;
	} else {
		thread->Messages = msg;
		thread->LastMessage = msg;
	}
	
	SHORTREL(&thread->IsLocked);
	
	SHORTLOCK(&glThreadListLock);
	LOG("Waking %p (%i %s)", thread, thread->TID, thread->ThreadName);
	Threads_Wake( thread );
	SHORTREL(&glThreadListLock);
	
	LEAVE_RET('i', 0);
}

/**
 * \fn int Proc_GetMessage(Uint *Err, Uint *Source, void *Buffer)
 * \brief Gets a message
 * \param Err	Pointer to \a errno
 * \param Source	Where to put the source TID
 * \param Buffer	Buffer to place the message data (set to NULL to just get message length)
 */
int Proc_GetMessage(Uint *Err, Uint *Source, void *Buffer)
{
	 int	ret;
	void	*tmp;
	tThread	*cur = Proc_GetCurThread();

	ENTER("pSource pBuffer", Source, Buffer);
	
	// Check if queue has any items
	if(!cur->Messages) {
		LEAVE('i', 0);
		return 0;
	}

	SHORTLOCK( &cur->IsLocked );
	
	if(Source) {
		*Source = cur->Messages->Source;
		LOG("*Source = %i", *Source);
	}
	
	// Get message length
	if( !Buffer ) {
		ret = cur->Messages->Length;
		SHORTREL( &cur->IsLocked );
		LEAVE('i', ret);
		return ret;
	}
	
	// Get message
	if(Buffer != GETMSG_IGNORE)
	{
		if( !CheckMem( Buffer, cur->Messages->Length ) )
		{
			LOG("Invalid buffer");
			*Err = -EINVAL;
			SHORTREL( &cur->IsLocked );
			LEAVE('i', -1);
			return -1;
		}
		LOG("Copied to buffer");
		memcpy(Buffer, cur->Messages->Data, cur->Messages->Length);
	}
	ret = cur->Messages->Length;
	
	// Remove from list
	tmp = cur->Messages;
	cur->Messages = cur->Messages->Next;
	if(cur->Messages == NULL)	cur->LastMessage = NULL;
	
	SHORTREL( &cur->IsLocked );
	
	free(tmp);	// Free outside of lock

	LEAVE('i', ret);
	return ret;
}
