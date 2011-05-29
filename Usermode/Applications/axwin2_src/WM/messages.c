/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"
#include <acess/sys.h>
#include <net.h>
#include <axwin/messages.h>
//#include <select.h>

#define AXWIN_PORT	4101

#define STATICBUF_SIZE	64

// === TYPES ===
typedef void tMessages_Handle_Callback(void*, size_t, void*);

// === PROTOTYPES ===
void	Messages_PollIPC();
void	Messages_RespondIPC(void *Ident, size_t Length, void *Data);
void	Messages_Handle(void *Ident, int MsgLen, tAxWin_Message *Msg, tMessages_Handle_Callback *Respond);

// === GLOBALS ===
 int	giIPCFileHandle;

// === CODE ===
void IPC_Init(void)
{
	 int	tmp;
	// TODO: Check this
	giIPCFileHandle = open("/Devices/ip/loop/udp", OPENFLAG_READ);
	tmp = AXWIN_PORT;	ioctl(giIPCFileHandle, 4, &tmp);	// TODO: Don't hard-code IOCtl number
}

void IPC_FillSelect(int *nfds, fd_set *set)
{
	if( giIPCFileHandle > *nfds )	*nfds = giIPCFileHandle;
	FD_SET(giIPCFileHandle, set);
}

void IPC_HandleSelect(fd_set *set)
{
	if( FD_ISSET(giIPCFileHandle, set) )
	{
		char	staticBuf[STATICBUF_SIZE];
		 int	readlen, identlen;
		char	*msg;
		readlen = read(giIPCFileHandle, sizeof(staticBuf), staticBuf);
		
		// Assume that all connections are from localhost
		identlen = 4 + Net_GetAddressSize( ((uint16_t*)staticBuf)[1] );
		msg = staticBuf + identlen;

		Messages_Handle(staticBuf, readlen - identlen, (void*)msg, Messages_RespondIPC);
	}
}

#if 0
void Messages_PollIPC()
{
	 int	len;
	pid_t	tid = 0;
	char	staticBuf[STATICBUF_SIZE];
	tAxWin_Message	*msg;
	
	// Wait for a message
	while( (len = SysGetMessage(&tid, NULL)) == 0 )
		sleep();
	
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
#endif

void Messages_RespondIPC(void *Ident, size_t Length, void *Data)
{
	SysSendMessage( *(tid_t*)Ident, Length, Data );
}

void Messages_Handle(void *Ident, int MsgLen, tAxWin_Message *Msg, tMessages_Handle_Callback *Respond)
{
	switch(Msg->ID)
	{
	#if 0
	case MSG_SREQ_PING:
		Msg->ID = MSG_SRSP_VERSION;
		Msg->Size = 2;
		Msg->Data[0] = 0;
		Msg->Data[1] = 1;
		*(uint16_t*)&Msg->Data[2] = -1;
		Messages_RespondIPC(ID, sizeof(Msg->ID), Msg);
		break;
	#endif
	default:
		fprintf(stderr, "WARNING: Unknown message %i (%p)\n", Msg->ID, Respond);
		_SysDebug("WARNING: Unknown message %i (%p)\n", Msg->ID, Respond);
		break;
	}
}

