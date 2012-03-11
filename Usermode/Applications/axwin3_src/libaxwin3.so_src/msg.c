/*
 * AxWin3 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * msg.c
 * - Message handling / IPC
 */
#include <axwin3/axwin.h>
#include <acess/sys.h>
#include <string.h>
#include <stdlib.h>
#include <ipcmessages.h>	// AxWin3 common
#include "include/internal.h"
#include "include/ipc.h"

// === CONSTANTS ===
enum eConnectionType
{
	CONNTYPE_SENDMESSAGE,
	CONNTYPE_UDP,
	CONNTYPE_TCP
};

// === GLOBALS ===
enum eConnectionType	giConnectionType;
int	giConnectionNum;	// FD or PID
char	gaAxWin3_int_UDPHeader[] = {5,16,0,0};	// Port 4101
 int	giAxWin3_int_UDPHeaderLen = sizeof(gaAxWin3_int_UDPHeader);
const char	*gsAxWin3_int_ServerDesc;
tAxWin3_MessageCallback	gAxWin3_MessageCallback;

// === CODE ===
void AxWin3_Connect(const char *ServerDesc)
{
	_SysDebug("ServerDesc = %s", ServerDesc);
	if( !ServerDesc )
	{
		ServerDesc = gsAxWin3_int_ServerDesc;
	}
	_SysDebug("ServerDesc = %s", ServerDesc);
	if( !ServerDesc )
	{
		// TODO: Error out
		return ;
	}
	switch(ServerDesc[0])
	{
	case '1': case '2': case '3': case '4': case '5':
	case '6': case '7': case '8': case '9': case '0':
		giConnectionType = CONNTYPE_SENDMESSAGE;
		giConnectionNum = atoi(ServerDesc);
		break;
	case 'u':
		while(*ServerDesc && *ServerDesc != ':')	ServerDesc ++;
		ServerDesc ++;
		// TODO: Open socket and create UDP header
		break;
	case 't':
		while(*ServerDesc && *ServerDesc != ':')	ServerDesc ++;
		ServerDesc ++;
		// TODO: Open socket
		break;
	}
}

tAxWin3_MessageCallback AxWin3_SetMessageCallback(tAxWin3_MessageCallback Callback)
{
	tAxWin3_MessageCallback	old = gAxWin3_MessageCallback;
	gAxWin3_MessageCallback = Callback;
	return old;
}

uint32_t AxWin3_int_GetWindowID(tHWND Window)
{
	if(Window)
		return Window->ServerID;
	else
		return -1;
}

tAxWin_IPCMessage *AxWin3_int_AllocateIPCMessage(tHWND Window, int Message, int Flags, int ExtraBytes)
{
	tAxWin_IPCMessage	*ret;

	ret = malloc( sizeof(tAxWin_IPCMessage) + ExtraBytes );
	ret->Flags = Flags;
	ret->ID = Message;
	ret->Window = AxWin3_int_GetWindowID(Window);
	ret->Size = ExtraBytes;
	return ret;
}

void AxWin3_int_SendIPCMessage(tAxWin_IPCMessage *Msg)
{
	 int	size = sizeof(tAxWin_IPCMessage) + Msg->Size;
	switch(giConnectionType)
	{
	case CONNTYPE_SENDMESSAGE:
		SysSendMessage(giConnectionNum, size, Msg);
		break;
	case CONNTYPE_UDP: {
		// Create UDP header
		char	tmpbuf[giAxWin3_int_UDPHeaderLen + size];
		memcpy(tmpbuf, gaAxWin3_int_UDPHeader, giAxWin3_int_UDPHeaderLen);
		memcpy(tmpbuf + giAxWin3_int_UDPHeaderLen, Msg, size);
		write(giConnectionNum, tmpbuf, sizeof(tmpbuf));
		}
		break;
	case CONNTYPE_TCP:
		write(giConnectionNum, Msg, size);
		break;
	default:
		break;
	}
}

tAxWin_IPCMessage *AxWin3_int_GetIPCMessage(void)
{
	 int	len;
	tAxWin_IPCMessage	*ret = NULL;
	switch(giConnectionType)
	{
	case CONNTYPE_SENDMESSAGE:
		_SysWaitEvent(THREAD_EVENT_IPCMSG);
		while(SysGetMessage(NULL, NULL))
		{
			pid_t	tid;
			len = SysGetMessage(&tid, NULL);
			// Check if the message came from the server
			if(tid != giConnectionNum)
			{
				_SysDebug("%i byte message from %i", len, tid);
				// If not, pass the buck (or ignore)
				if( gAxWin3_MessageCallback )
					gAxWin3_MessageCallback(tid, len);
				else
					SysGetMessage(NULL, GETMSG_IGNORE);
				continue ;
			}
			
			// If it's from the server, allocate a buffer and return it
			ret = malloc(len);
			if(ret == NULL) {
				_SysDebug("malloc() failed, ignoring message");
				SysGetMessage(NULL, GETMSG_IGNORE);
				return NULL;
			}
			SysGetMessage(NULL, ret);
			break;
		}
		break;
	default:
		// TODO: Implement
		_SysDebug("TODO: Implement AxWin3_int_GetIPCMessage for TCP/UDP");
		break;
	}

	// No message?
	if( ret == NULL )
		return NULL;

	// TODO: Sanity checks, so a stupid server can't crash us

	return ret;
}

tAxWin_IPCMessage *AxWin3_int_WaitIPCMessage(int WantedID)
{
	tAxWin_IPCMessage	*msg;
	for(;;)
	{
		msg = AxWin3_int_GetIPCMessage();
		if(msg->ID == WantedID)	return msg;
		AxWin3_int_HandleMessage( msg );
		free(msg);
	}
}

