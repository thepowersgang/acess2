/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"
#include <acess/sys.h>
#include <axwin/messages.h>

#define STATICBUF_SIZE	64

// === TYPES ===
typedef void tMessages_Handle_Callback(int, size_t,void*);

// === PROTOTYPES ===
void	Messages_PollIPC();
void	Messages_RespondIPC(int ID, size_t Length, void *Data);
void	Messages_Handle(tAxWin_Message *Msg, tMessages_Handle_Callback *Respond, int ID);

// === GLOBALS ===

// === CODE ===
void Messages_PollIPC()
{
	 int	len;
	 int	tid = 0;
	char	staticBuf[STATICBUF_SIZE];
	tAxWin_Message	*msg;
	
	// Wait for a message
	while( (len = SysGetMessage(&tid, NULL)) )
		yield();
	
	// Allocate the space for it
	if( len <= STATICBUF_SIZE )
		msg = (void*)staticBuf;
	else {
		msg = malloc( len );
		if(!msg) {
			fprintf(
				stderr,
				"ERROR - Unable to allocate message buffer, ignoring message from %i\n",
				tid);
			SysGetMessage(NULL, GETMSG_IGNORE);
			return ;
		}
	}
	
	// Get message data
	SysGetMessage(NULL, msg);
	
	Messages_Handle(msg, Messages_RespondIPC, tid);
}

void Messages_RespondIPC(int ID, size_t Length, void *Data)
{
	SysSendMessage(ID, Length, Data);
}

void Messages_Handle(tAxWin_Message *Msg, tMessages_Handle_Callback *Respond, int ID)
{
	switch(Msg->ID)
	{
	case MSG_REQ_PING:
		Msg->ID = MSG_RSP_PONG;
		Respond(ID, sizeof(Msg->ID), Msg);
		break;
	default:
		fprintf(stderr, "WARNING: Unknown message %i from %i (%p)\n",
			Msg->ID, ID, Respond);
		break;
	}
}

