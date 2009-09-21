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
	thread = Proc_GetThread( Dest );
	
	// Error check
	if(!thread) {	return -1;	}
	
	// Get Spinlock
	LOCK( &thread->IsLocked );
	
	// Check if thread is still alive
	if(thread->Status == THREAD_STAT_DEAD)	return -1;
	
	// Create message
	msg = malloc( sizeof(tMsg)+Length );
	msg->Next = NULL;
	msg->Source = gCurrentThread->TID;
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
	
	Thread_Wake( thread );
	
	return 0;
}

/**
 * \fn int Proc_GetMessage(Uint *Err, Uint *Source, void *Buffer)
 * \brief Gets a message
 */
int Proc_GetMessage(Uint *Err, Uint *Source, void *Buffer)
{
	 int	ret;
	void *tmp;
	
	// Check if queue has any items
	if(!gCurrentThread->Messages) {
		return 0;
	}

	LOCK( &gCurrentThread->IsLocked );
	
	if(Source)
		*Source = gCurrentThread->Messages->Source;
	
	// Get message length
	if( !Buffer ) {
		ret = gCurrentThread->Messages->Length;
		RELEASE( &gCurrentThread->IsLocked );
		return ret;
	}
	
	// Get message
	if(Buffer != GETMSG_IGNORE)
		memcpy(Buffer, gCurrentThread->Messages->Data, gCurrentThread->Messages->Length);
	ret = gCurrentThread->Messages->Length;
	
	// Remove from list
	tmp = gCurrentThread->Messages->Next;
	free(gCurrentThread->Messages);
	gCurrentThread->Messages = tmp;
	
	RELEASE( &gCurrentThread->IsLocked );
	
	return ret;
}
