/*
 * AcessOS Microkernel Version
 * messages.c
 */
#include <common.h>
#include <proc.h>
#include <errno.h>

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
	
	Log("Proc_SendMessage: (Err=%p, Dest=%i, Length=%i, Data=%p)", Err, Dest, Length, Data);
	
	if(Length <= 0 || !Data) {
		*Err = -EINVAL;
		return -1;
	}
	
	// Get thread
	thread = Threads_GetThread( Dest );
	
	// Error check
	if(!thread) {	return -1;	}
	
	// Get Spinlock
	LOCK( &thread->IsLocked );
	
	// Check if thread is still alive
	if(thread->Status == THREAD_STAT_DEAD)	return -1;
	
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
	
	RELEASE(&thread->IsLocked);
	
	Threads_Wake( thread );
	
	return 0;
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
	void *tmp;
	tThread	*cur = Proc_GetCurThread();
	
	// Check if queue has any items
	if(!cur->Messages) {
		return 0;
	}

	LOCK( &cur->IsLocked );
	
	if(Source)
		*Source = cur->Messages->Source;
	
	// Get message length
	if( !Buffer ) {
		ret = cur->Messages->Length;
		RELEASE( &cur->IsLocked );
		return ret;
	}
	
	// Get message
	if(Buffer != GETMSG_IGNORE)
		memcpy(Buffer, cur->Messages->Data, cur->Messages->Length);
	ret = cur->Messages->Length;
	
	// Remove from list
	tmp = cur->Messages->Next;
	free( (void*)cur->Messages );
	cur->Messages = tmp;
	
	RELEASE( &cur->IsLocked );
	
	return ret;
}
