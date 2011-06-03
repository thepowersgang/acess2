/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"
#include <acess/sys.h>
#include <net.h>
#include <axwin2/messages.h>
#include <string.h>

#define AXWIN_PORT	4101

#define STATICBUF_SIZE	64

// === TYPES ===

// === PROTOTYPES ===
void	IPC_Init(void);
void	IPC_FillSelect(int *nfds, fd_set *set);
void	IPC_HandleSelect(fd_set *set);
void	Messages_RespondDatagram(void *Ident, size_t Length, void *Data);
void	Messages_RespondIPC(void *Ident, size_t Length, void *Data);
void	Messages_Handle(void *Ident, size_t MsgLen, tAxWin_Message *Msg, tMessages_Handle_Callback *Respond);

// === GLOBALS ===
 int	giNetworkFileHandle = -1;
 int	giMessagesFileHandle = -1;

// === CODE ===
void IPC_Init(void)
{
	 int	tmp;
	// TODO: Check this
	giNetworkFileHandle = open("/Devices/ip/loop/udp", OPENFLAG_READ);
	tmp = AXWIN_PORT;	ioctl(giNetworkFileHandle, 4, &tmp);	// TODO: Don't hard-code IOCtl number

	// TODO: Open a handle to something like /Devices/proc/cur/messages to watch for messages
//	giMessagesFileHandle = open("/Devices/"
}

void IPC_FillSelect(int *nfds, fd_set *set)
{
	if( giNetworkFileHandle > *nfds )	*nfds = giNetworkFileHandle;
	FD_SET(giNetworkFileHandle, set);
}

void IPC_HandleSelect(fd_set *set)
{
	if( FD_ISSET(giNetworkFileHandle, set) )
	{
		char	staticBuf[STATICBUF_SIZE];
		 int	readlen, identlen;
		char	*msg;

		readlen = read(giNetworkFileHandle, sizeof(staticBuf), staticBuf);
		
		// Assume that all connections are from localhost
		identlen = 4 + Net_GetAddressSize( ((uint16_t*)staticBuf)[1] );
		msg = staticBuf + identlen;

		Messages_Handle(staticBuf, readlen - identlen, (void*)msg, Messages_RespondDatagram);
	}

	while(SysGetMessage(NULL, NULL))
	{
		pid_t	tid;
		 int	len = SysGetMessage(&tid, NULL);
		char	data[len];
		SysGetMessage(NULL, data);

		Messages_Handle(&tid, len, (void*)data, Messages_RespondIPC);
	}
}

void Messages_RespondDatagram(void *Ident, size_t Length, void *Data)
{
	 int	addrSize = Net_GetAddressSize( ((uint16_t*)Ident)[1] );
	char	tmpbuf[ 4 + addrSize + Length ];
	memcpy(tmpbuf, Ident, 4 + addrSize);
	memcpy(tmpbuf + 4 + addrSize, Data, Length);
	// TODO: Handle fragmented packets
	write(giNetworkFileHandle, sizeof(tmpbuf), tmpbuf);
}

void Messages_RespondIPC(void *Ident, size_t Length, void *Data)
{
	SysSendMessage( *(tid_t*)Ident, Length, Data );
}

void Messages_Handle(void *Ident, size_t MsgLen, tAxWin_Message *Msg, tMessages_Handle_Callback *Respond)
{
	if( MsgLen < sizeof(tAxWin_Message) )
		return ;
	if( MsgLen < sizeof(tAxWin_Message) + Msg->Size )
		return ;

	switch(Msg->ID)
	{
	case MSG_SREQ_PING:
		if( MsgLen < sizeof(tAxWin_Message) + 4 )	return;
		Msg->ID = MSG_SRSP_VERSION;
		Msg->Size = 4;
		Msg->Data[0] = 0;
		Msg->Data[1] = 1;
		*(uint16_t*)&Msg->Data[2] = -1;
		Respond(Ident, sizeof(Msg->ID), Msg);
		break;

	case MSG_SREQ_REGISTER:
		if( Msg->Len == strnlen(Msg->Len, Msg->Data) ) {
			// Special handling?
			return ;
		}
		
		break;

	default:
		fprintf(stderr, "WARNING: Unknown message %i (%p)\n", Msg->ID, Respond);
		_SysDebug("WARNING: Unknown message %i (%p)\n", Msg->ID, Respond);
		break;
	}
}

