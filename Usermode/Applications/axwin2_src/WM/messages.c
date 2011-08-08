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
void	IPC_Handle(tIPC_Type *IPCType, void *Ident, size_t MsgLen, tAxWin_Message *Msg);
void	IPC_ReturnValue(tIPC_Type *IPCType, void *Ident, int MessageID, uint32_t Value);
 int	IPC_Type_Datagram_GetSize(void *Ident);
 int	IPC_Type_Datagram_Compare(void *Ident1, void *Ident2);
void	IPC_Type_Datagram_Send(void *Ident, size_t Length, void *Data);
 int	IPC_Type_Sys_GetSize(void *Ident);
 int	IPC_Type_Sys_Compare(void *Ident1, void *Ident2);
void	IPC_Type_Sys_Send(void *Ident, size_t Length, void *Data);

// === GLOBALS ===
 int	giNetworkFileHandle = -1;
 int	giMessagesFileHandle = -1;
tIPC_Type	gIPC_Type_Datagram = {
	IPC_Type_Datagram_GetSize,
	IPC_Type_Datagram_Compare, 
	IPC_Type_Datagram_Send
};
tIPC_Type	gIPC_Type_SysMessage = {
	IPC_Type_Sys_GetSize,
	IPC_Type_Sys_Compare,
	IPC_Type_Sys_Send
};

// === CODE ===
void IPC_Init(void)
{
	 int	tmp;
	// TODO: Check this
	giNetworkFileHandle = open("/Devices/ip/loop/udp", OPENFLAG_READ);
	tmp = AXWIN_PORT;	ioctl(giNetworkFileHandle, 4, &tmp);	// TODO: Don't hard-code IOCtl number
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

		readlen = read(giNetworkFileHandle, staticBuf, sizeof(staticBuf));
		
		identlen = 4 + Net_GetAddressSize( ((uint16_t*)staticBuf)[1] );
		msg = staticBuf + identlen;

		IPC_Handle(&gIPC_Type_Datagram, staticBuf, readlen - identlen, (void*)msg);
		_SysDebug("IPC_HandleSelect: UDP handled");
	}

	while(SysGetMessage(NULL, NULL))
	{
		pid_t	tid;
		 int	len = SysGetMessage(&tid, NULL);
		char	data[len];
		SysGetMessage(NULL, data);

		IPC_Handle(&gIPC_Type_SysMessage, &tid, len, (void*)data);
		_SysDebug("IPC_HandleSelect: Message handled");
	}
}

void IPC_Handle(tIPC_Type *IPCType, void *Ident, size_t MsgLen, tAxWin_Message *Msg)
{
	tApplication	*app;
	tElement	*ele;
	
	_SysDebug("IPC_Handle: (IPCType=%p, Ident=%p, MsgLen=%i, Msg=%p)",
		IPCType, Ident, MsgLen, Msg);
	
	if( MsgLen < sizeof(tAxWin_Message) )
		return ;
	if( MsgLen < sizeof(tAxWin_Message) + Msg->Size )
		return ;
	
	app = AxWin_GetClient(IPCType, Ident);

	switch((enum eAxWin_Messages) Msg->ID)
	{
	// --- Ping message (reset timeout and get server version)
	case MSG_SREQ_PING:
		_SysDebug(" IPC_Handle: MSG_SREQ_PING");
		if( MsgLen < sizeof(tAxWin_Message) + 4 )	return;
		Msg->ID = MSG_SRSP_VERSION;
		Msg->Size = 4;
		Msg->Data[0] = 0;
		Msg->Data[1] = 1;
		*(uint16_t*)&Msg->Data[2] = -1;
		IPCType->SendMessage(Ident, sizeof(Msg->ID), Msg);
		break;


	// --- Register an application
	case MSG_SREQ_REGISTER:
		_SysDebug(" IPC_Handle: MSG_SREQ_REGISTER");
		if( Msg->Data[Msg->Size-1] != '\0' ) {
			// Invalid message
			_SysDebug("IPC_Handle: RETURN - Not NULL terminated");
			return ;
		}
		
		if( app != NULL ) {
			_SysDebug("Notice: Duplicate registration (%s)\n", Msg->Data);
			return ;
		}
		
		// TODO: Should this function be implemented here?
		AxWin_RegisterClient(IPCType, Ident, Msg->Data);
		break;
	
	// --- Create a window
	case MSG_SREQ_ADDWIN:
		_SysDebug(" IPC_Handle: MSG_SREQ_ADDWIN");
		if( Msg->Data[Msg->Size-1] != '\0' ) {
			// Invalid message
			return ;
		}
		
		ele = AxWin_CreateAppWindow(app, Msg->Data);
		IPC_ReturnValue(IPCType, Ident, MSG_SREQ_ADDWIN, ele->ApplicationID);
		break;
	
	// --- Set a window's icon
	case MSG_SREQ_SETICON:
		_SysDebug(" IPC_Handle: MSG_SREQ_SETICON");
		// TODO: Find a good way of implementing this
		break;
	
	// --- Create an element
	case MSG_SREQ_INSERT: {
		_SysDebug(" IPC_Handle: MSG_SREQ_INSERT");
		struct sAxWin_SReq_NewElement	*info = (void *)Msg->Data;
		
		if( Msg->Size != sizeof(*info) )	return;
		
		if( !app || info->Parent > app->MaxElementIndex )	return ;
		
		ele = AxWin_CreateElement( app->EleIndex[info->Parent], info->Type, info->Flags, NULL );
		IPC_ReturnValue(IPCType, Ident, MSG_SREQ_ADDWIN, ele->ApplicationID);
		break; }
	
	// --- Unknown message
	default:
		fprintf(stderr, "WARNING: Unknown message %i (%p)\n", Msg->ID, IPCType);
		_SysDebug("WARNING: Unknown message %i (%p)\n", Msg->ID, IPCType);
		break;
	}
}

void IPC_ReturnValue(tIPC_Type *IPCType, void *Ident, int MessageID, uint32_t Value)
{
	char	data[sizeof(tAxWin_Message) + sizeof(tAxWin_RetMsg)];
	tAxWin_Message	*msg = (void *)data;
	tAxWin_RetMsg	*ret_msg = (void *)msg->Data;
	
	msg->Source = 0;	// 0 = Server
	msg->ID = MSG_SRSP_RETURN;
	msg->Size = sizeof(tAxWin_RetMsg);
	ret_msg->ReqID = MessageID;
	ret_msg->Rsvd = 0;
	ret_msg->Value = Value;
	
	IPCType->SendMessage(Ident, sizeof(data), data);
}

int IPC_Type_Datagram_GetSize(void *Ident)
{
	return 4 + Net_GetAddressSize( ((uint16_t*)Ident)[1] );
}

int IPC_Type_Datagram_Compare(void *Ident1, void *Ident2)
{
	// Pass the buck :)
	// - No need to worry about mis-matching sizes, as the size is computed
	//   from the 3rd/4th bytes, hence it will differ before the size is hit.
	return memcmp(Ident1, Ident2, IPC_Type_Datagram_GetSize(Ident1));
}

void IPC_Type_Datagram_Send(void *Ident, size_t Length, void *Data)
{
	 int	identlen = IPC_Type_Datagram_GetSize(Ident);
	char	tmpbuf[ identlen + Length ];
	memcpy(tmpbuf, Ident, identlen);	// Header
	memcpy(tmpbuf + identlen, Data, Length);	// Data
	// TODO: Handle fragmented packets
	write(giNetworkFileHandle, tmpbuf, sizeof(tmpbuf));
}

int IPC_Type_Sys_GetSize(void *Ident)
{
	return sizeof(pid_t);
}

int IPC_Type_Sys_Compare(void *Ident1, void *Ident2)
{
	return *(int*)Ident1 - *(int*)Ident2;
}

void IPC_Type_Sys_Send(void *Ident, size_t Length, void *Data)
{
	SysSendMessage( *(tid_t*)Ident, Length, Data );
}
