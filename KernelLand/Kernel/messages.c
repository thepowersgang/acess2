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
#include <events.h>

// === CODE ===
/**
 * \fn int Proc_SendMessage(Uint Dest, int Length, void *Data)
 * \brief Send an IPC message
 * \param Dest	Destination Thread
 * \param Length	Length of the message
 * \param Data	Message data
 */
int Proc_SendMessage(Uint Dest, int Length, void *Data)
{
	tThread	*thread;
	tMsg	*msg;
	
	ENTER("iDest iLength pData", Dest, Length, Data);
	
	if(Length <= 0 || !Data) {
		errno = -EINVAL;
		LEAVE_RET('i', -1);
	}
	
	// TODO: Check message length against global/per-thread maximums
	// TODO: Restrict queue length

	// Get thread
	thread = Threads_GetThread( Dest );
	if(!thread)	LEAVE_RET('i', -1);
	LOG("Destination %p(%i %s)", thread, thread->TID, thread->ThreadName);
	
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

	// Wake the thread	
	LOG("Waking %p (%i %s)", thread, thread->TID, thread->ThreadName);
	Threads_PostEvent( thread, THREAD_EVENT_IPCMSG );
	
	LEAVE_RET('i', 0);
}

/**
 * \fn int Proc_GetMessage(Uint *Source, void *Buffer)
 * \brief Gets a message
 * \param Source	Where to put the source TID
 * \param BufSize	Size of \a Buffer, only this many bytes will be copied
 * \param Buffer	Buffer to place the message data (set to NULL to just get message length)
 * \return Message length
 */
int Proc_GetMessage(Uint *Source, Uint BufSize, void *Buffer)
{
	 int	ret;
	void	*tmp;
	tThread	*cur = Proc_GetCurThread();

	ENTER("pSource xBufSize pBuffer", Source, BufSize, Buffer);
	
	// Check if queue has any items
	if(!cur->Messages) {
		LOG("empty queue");
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
		if( !CheckMem( Buffer, BufSize ) )
		{
			LOG("Invalid buffer");
			errno = -EINVAL;
			SHORTREL( &cur->IsLocked );
			LEAVE('i', -1);
			return -1;
		}
		if( BufSize < cur->Messages->Length )
			Log_Notice("Threads", "Buffer of 0x%x passed, but 0x%x long message, truncated",
				BufSize, cur->Messages->Length);
		else if( BufSize < cur->Messages->Length )
			BufSize = cur->Messages->Length;
		else
			;	// equal
		LOG("Copied to buffer");
		memcpy(Buffer, cur->Messages->Data, BufSize);
	}
	ret = cur->Messages->Length;
	
	// Remove from list
	tmp = cur->Messages;
	cur->Messages = cur->Messages->Next;
	// - Removed last message? Clear the end-of-list pointer
	if(cur->Messages == NULL)	cur->LastMessage = NULL;
	//  > Otherwise, re-mark the IPCMSG event flag
	else	cur->EventState |= THREAD_EVENT_IPCMSG;
	
	SHORTREL( &cur->IsLocked );

	free(tmp);	// Free outside of lock

	LEAVE('i', ret);
	return ret;
}
