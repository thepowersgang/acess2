/*
 * Acess2 GUI (AxWin) Version 3
 * - By John Hodge (thePowersGang)
 * 
 * ipc.c
 * - Interprocess communication
 */
#include <common.h>
#include <acess/sys.h>
#include <net.h>
#include <string.h>
#include <ipcmessages.h>
#include <stdio.h>

#define AXWIN_PORT	4101

#define STATICBUF_SIZE	64

// === TYPES ===
typedef struct sIPC_Type	tIPC_Type;
struct sIPC_Type
{
	 int	(*GetIdentSize)(void *Ident);
	 int	(*CompareIdent)(void *Ident1, void *Ident2);
	void	(*SendMessage)(void *Ident, size_t, void *Data);
};

// === PROTOTYPES ===
void	IPC_Init(void);
void	IPC_FillSelect(int *nfds, fd_set *set);
void	IPC_HandleSelect(fd_set *set);
void	IPC_Handle(tIPC_Type *IPCType, void *Ident, size_t MsgLen, tAxWin_IPCMessage *Msg);
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

void IPC_Handle(tIPC_Type *IPCType, void *Ident, size_t MsgLen, tAxWin_IPCMessage *Msg)
{
	_SysDebug("IPC_Handle: (IPCType=%p, Ident=%p, MsgLen=%i, Msg=%p)",
		IPCType, Ident, MsgLen, Msg);
	
	if( MsgLen < sizeof(tAxWin_IPCMessage) )
		return ;
	if( MsgLen < sizeof(tAxWin_IPCMessage) + Msg->Size )
		return ;
	
//	win = AxWin_GetClient(IPCType, Ident, Msg->Window);

	switch((enum eAxWin_IPCMessageTypes) Msg->ID)
	{
	// --- Ping message (reset timeout and get server version)
	case IPCMSG_PING:
		_SysDebug(" IPC_Handle: IPCMSG_PING");
		if( MsgLen < sizeof(tAxWin_IPCMessage) + 4 )	return;
		if( Msg->Flags & IPCMSG_FLAG_RETURN )
		{
			Msg->ID = IPCMSG_PING;
			Msg->Size = sizeof(tIPCMsg_Return);
			((tIPCMsg_Return*)Msg->Data)->Value = AXWIN_VERSION;
			IPCType->SendMessage(Ident, sizeof(tIPCMsg_Return), Msg);
		}
		break;

	// --- 

	// --- Unknown message
	default:
		fprintf(stderr, "WARNING: Unknown message %i (%p)\n", Msg->ID, IPCType);
		_SysDebug("WARNING: Unknown message %i (%p)\n", Msg->ID, IPCType);
		break;
	}
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
