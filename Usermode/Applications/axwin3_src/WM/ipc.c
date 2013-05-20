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
#include <wm.h>
#include <wm_internals.h>
#include <wm_hotkeys.h>	// Hotkey registration
#include <wm_renderer.h>	// Renderer IPC messages

#define AXWIN_PORT	4101

#define STATICBUF_SIZE	64
#define MAX_WINDOWS_PER_APP	128

// === TYPES ===
typedef struct sIPC_Type	tIPC_Type;

struct sIPC_Type
{
	 int	(*GetIdentSize)(const void *Ident);
	 int	(*CompareIdent)(const void *Ident1, const void *Ident2);
	void	(*SendMessage)(const void *Ident, size_t Length, const void *Data);
};

struct sIPC_Client
{
	const tIPC_Type	*IPCType;
	const void	*Ident;	// Stored after structure

	 int	nWindows;
	tWindow	**Windows;
};

// === IMPORTS ===
extern tWindow	*gpWM_FocusedWindow;	// Needed for _FocusWindow

// === PROTOTYPES ===
void	IPC_Init(void);
void	IPC_FillSelect(int *nfds, fd_set *set);
void	IPC_HandleSelect(fd_set *set);
 int	IPC_Type_Datagram_GetSize(const void *Ident);
 int	IPC_Type_Datagram_Compare(const void *Ident1, const void *Ident2);
void	IPC_Type_Datagram_Send(const void *Ident, size_t Length, const void *Data);
 int	IPC_Type_Sys_GetSize(const void *Ident);
 int	IPC_Type_Sys_Compare(const void *Ident1, const void *Ident2);
void	IPC_Type_Sys_Send(const void *Ident, size_t Length, const void *Data);
 int	IPC_Type_IPCPipe_GetSize(const void *Ident);
 int	IPC_Type_IPCPipe_Compare(const void *Ident1, const void *Ident2);
void	IPC_Type_IPCPipe_Send(const void *Ident, size_t Length, const void *Data);
tIPC_Client	*IPC_int_GetClient(const tIPC_Type *IPCType, const void *Ident);
void	IPC_int_DropClient(tIPC_Client *Client);
void	IPC_Handle(tIPC_Client *Client, size_t MsgLen, tAxWin_IPCMessage *Msg);

// === GLOBALS ===
const tIPC_Type	gIPC_Type_Datagram = {
	IPC_Type_Datagram_GetSize,
	IPC_Type_Datagram_Compare, 
	IPC_Type_Datagram_Send
};
const tIPC_Type	gIPC_Type_SysMessage = {
	IPC_Type_Sys_GetSize,
	IPC_Type_Sys_Compare,
	IPC_Type_Sys_Send
};
const tIPC_Type	gIPC_Type_IPCPipe = {
	IPC_Type_IPCPipe_GetSize,
	IPC_Type_IPCPipe_Compare,
	IPC_Type_IPCPipe_Send
};
 int	giNetworkFileHandle = -1;
 int	giIPCPipeHandle = -1;
 int	giIPC_ClientCount;
tIPC_Client	**gIPC_Clients;

// === CODE ===
void IPC_Init(void)
{
	 int	tmp;
	// TODO: Check this
	giNetworkFileHandle = _SysOpen("/Devices/ip/loop/udp", OPENFLAG_READ);
	if( giNetworkFileHandle != -1 )
	{
		tmp = AXWIN_PORT;
		_SysIOCtl(giNetworkFileHandle, 4, &tmp);	// TODO: Don't hard-code IOCtl number
	}
	
	giIPCPipeHandle = _SysOpen("/Devices/ipcpipe/axwin"/*-$USER*/, OPENFLAG_CREATE);
	_SysDebug("giIPCPipeHandle = %i", giIPCPipeHandle);
	if( giIPCPipeHandle == -1 )
		_SysDebug("ERROR: Can't create IPCPipe handle");
}

void _setfd(int fd, int *nfds, fd_set *set)
{
	if( fd >= 0 )
	{
		if( fd >= *nfds )	*nfds = fd+1;
		FD_SET(fd, set);
	}
}

void IPC_FillSelect(int *nfds, fd_set *set)
{
	_setfd(giNetworkFileHandle, nfds, set);
	_setfd(giIPCPipeHandle, nfds, set);
	for( int i = 0; i < giIPC_ClientCount; i ++ )
	{
		if( gIPC_Clients[i] && gIPC_Clients[i]->IPCType == &gIPC_Type_IPCPipe )
			_setfd( *(int*)(gIPC_Clients[i]->Ident), nfds, set );
	}
}

void IPC_HandleSelect(fd_set *set)
{
	if( giNetworkFileHandle != -1 && FD_ISSET(giNetworkFileHandle, set) )
	{
		char	staticBuf[STATICBUF_SIZE];
		 int	readlen, identlen;
		char	*msg;

		readlen = _SysRead(giNetworkFileHandle, staticBuf, sizeof(staticBuf));
		
		identlen = 4 + Net_GetAddressSize( ((uint16_t*)staticBuf)[1] );
		msg = staticBuf + identlen;

		IPC_Handle( IPC_int_GetClient(&gIPC_Type_Datagram, staticBuf), readlen - identlen, (void*)msg);
		//_SysDebug("IPC_HandleSelect: UDP handled");
	}
	
	if( giIPCPipeHandle != -1 && FD_ISSET(giIPCPipeHandle, set) )
	{
		int newfd = _SysOpenChild(giIPCPipeHandle, "newclient", OPENFLAG_READ|OPENFLAG_WRITE);
		_SysDebug("newfd = %i", newfd);
		IPC_int_GetClient(&gIPC_Type_IPCPipe, &newfd);
	}
	
	for( int i = 0; i < giIPC_ClientCount; i ++ )
	{
		if( gIPC_Clients[i] && gIPC_Clients[i]->IPCType == &gIPC_Type_IPCPipe )
		{
			 int fd = *(const int*)gIPC_Clients[i]->Ident;
			if( FD_ISSET(fd, set) )
			{
				char	staticBuf[STATICBUF_SIZE];
				size_t	len;
				len = _SysRead(fd, staticBuf, sizeof(staticBuf));
				if( len == (size_t)-1 ) {
					// TODO: Check errno for EINTR
					IPC_int_DropClient(gIPC_Clients[i]);
					break;
				}
				IPC_Handle( gIPC_Clients[i], len, (void*)staticBuf );
			}
		}
	}

	size_t	len;
	int	tid;
	while( (len = _SysGetMessage(&tid, 0, NULL)) )
	{
		char	data[len];
		_SysGetMessage(NULL, len, data);

		IPC_Handle( IPC_int_GetClient(&gIPC_Type_SysMessage, &tid), len, (void*)data );
//		_SysDebug("IPC_HandleSelect: Message handled");
	}
}

int IPC_Type_Datagram_GetSize(const void *Ident)
{
	return 4 + Net_GetAddressSize( ((const uint16_t*)Ident)[1] );
}

int IPC_Type_Datagram_Compare(const void *Ident1, const void *Ident2)
{
	// Pass the buck :)
	// - No need to worry about mis-matching sizes, as the size is computed
	//   from the 3rd/4th bytes, hence it will differ before the size is hit.
	return memcmp(Ident1, Ident2, IPC_Type_Datagram_GetSize(Ident1));
}

void IPC_Type_Datagram_Send(const void *Ident, size_t Length, const void *Data)
{
	 int	identlen = IPC_Type_Datagram_GetSize(Ident);
	char	tmpbuf[ identlen + Length ];
	memcpy(tmpbuf, Ident, identlen);	// Header
	memcpy(tmpbuf + identlen, Data, Length);	// Data
	// TODO: Handle fragmented packets
	_SysWrite(giNetworkFileHandle, tmpbuf, sizeof(tmpbuf));
}

int IPC_Type_Sys_GetSize(const void *Ident)
{
	return sizeof(pid_t);
}

int IPC_Type_Sys_Compare(const void *Ident1, const void *Ident2)
{
	return *(const tid_t*)Ident1 - *(const tid_t*)Ident2;
}

void IPC_Type_Sys_Send(const void *Ident, size_t Length, const void *Data)
{
	_SysSendMessage( *(const tid_t*)Ident, Length, Data );
}

int IPC_Type_IPCPipe_GetSize(const void *Ident)
{
	return sizeof(int);
}
int IPC_Type_IPCPipe_Compare(const void *Ident1, const void *Ident2)
{
	return *(const int*)Ident1 - *(const int*)Ident2;
}
void IPC_Type_IPCPipe_Send(const void *Ident, size_t Length, const void *Data)
{
	size_t rv = _SysWrite( *(const int*)Ident, Data, Length );
	if(rv != Length) {
		_SysDebug("Sent message oversize %x", Length);
	}
}

// --- Client -> Window Mappings
int _CompareClientPtrs(const void *_a, const void *_b)
{
	tIPC_Client	*a = *(tIPC_Client**)_a;
	tIPC_Client	*b = *(tIPC_Client**)_b;

	ASSERT(a);
	if(!b)	return -1;

	if(a->IPCType < b->IPCType)	return -1;
	if(a->IPCType > b->IPCType)	return 1;
	
	return a->IPCType->CompareIdent(a->Ident, b->Ident);
}

int IPC_int_BSearchClients(const tIPC_Client *TargetClient, int *Pos)
{
	 int	div;
	 int	cmp = -1;
	 int	pos = 0;

	div = giIPC_ClientCount;
	pos = div/2;
	while(div > 0)
	{
		div /= 2;
		_SysDebug("Cmp with %i [%i] (%p)", pos, div, gIPC_Clients[pos]);
		cmp = _CompareClientPtrs(&TargetClient, &gIPC_Clients[pos]);
//		_SysDebug("Checking against %i gives %i", pos, cmp);
		if(cmp == 0)	break;
		if(cmp < 0)
			pos -= div;
		else
			pos += div;
	}
	
	// - Return if found	
	if(cmp == 0) {
		*Pos = pos;
		return 1;
	}

	// Adjust pos to be the index where the new client will be placed
	if(cmp > 0)	pos ++;
	*Pos = pos;
	return 0;
}

tIPC_Client *IPC_int_GetClient(const tIPC_Type *IPCType, const void *Ident)
{
	 int	pos = 0;	// Position where the new client will be inserted
	 int	ident_size;
	tIPC_Client	*ret;

	// - Search list of registered clients
	if(giIPC_ClientCount > 0)
	{
		tIPC_Client	target;
		target.IPCType = IPCType;
		target.Ident = Ident;
		if( IPC_int_BSearchClients(&target, &pos) )
			return gIPC_Clients[pos];
	}


	// - Create a new client entry
	ident_size = IPCType->GetIdentSize(Ident);
//	_SysDebug("ident_size = %i", ident_size);
	ret = malloc( sizeof(tIPC_Client) + ident_size );
	if(!ret)	return NULL;
	ret->IPCType = IPCType;
	ret->Ident = ret + 1;	// Get the end of the structure
	memcpy( (void*)ret->Ident, Ident, ident_size );
	ret->nWindows = 0;
	ret->Windows = NULL;
	
	// TODO: Register some way of detecting the client disconnecting
	//       > Wait on the thread / register with kernel somehow
	//       > Sockets are easier, but UDP is harder. Might get rid of it

	// - Insert
	giIPC_ClientCount ++;
	gIPC_Clients = realloc(gIPC_Clients, giIPC_ClientCount*sizeof(tIPC_Client*));
	memmove(&gIPC_Clients[pos+1], &gIPC_Clients[pos], (giIPC_ClientCount-pos-1) * sizeof(tIPC_Client*));
	gIPC_Clients[pos] = ret;

	return ret;
}

void IPC_int_DropClient(tIPC_Client *Client)
{
	// Remove from client list
	 int	pos;
	if( !IPC_int_BSearchClients(Client, &pos) ) {
		_SysDebug("IPC_int_DropClient: Can't find client %p", Client);
		return ;
	}

	giIPC_ClientCount --;
	memmove(&gIPC_Clients[pos], &gIPC_Clients[pos+1], (giIPC_ClientCount-pos)*sizeof(tIPC_Client*));

	// Terminate client's windows
	// - If there were active windows, show an error?
	 int	nWindowsDestroyed = 0;
	for(int i = 0; i < Client->nWindows; i ++)
	{
		if( Client->Windows[i] )
		{
			_SysDebug("Window %p:%i %p still exists", Client, i, Client->Windows[i]);
			WM_DestroyWindow(Client->Windows[i]);
			nWindowsDestroyed ++;
		}
	}
	if( nWindowsDestroyed )
	{
		_SysDebug("TODO: Show notice that application exited without destroying windows");
	}
	
	// Free client structure
	free(Client);
	_SysDebug("Dropped client %p", Client);
}

tWindow *IPC_int_GetWindow(tIPC_Client *Client, uint32_t WindowID)
{
	if( WindowID == -1 )
		return NULL;

	if( WindowID >= Client->nWindows ) {
		return NULL;
	}

	return Client->Windows[WindowID];
}

void IPC_int_SetWindow(tIPC_Client *Client, uint32_t WindowID, tWindow *WindowPtr)
{
	if( WindowID >= MAX_WINDOWS_PER_APP )
		return ;

	if( WindowID >= Client->nWindows )
	{
		 int	oldCount = Client->nWindows;
		Client->nWindows = WindowID + 1;
		Client->Windows = realloc(Client->Windows, Client->nWindows*sizeof(tWindow*));
		memset( &Client->Windows[oldCount],  0, (Client->nWindows-oldCount)*sizeof(tWindow*) );
		_SysDebug("Expanded %p's window list from %i to %i", Client, oldCount, Client->nWindows);
	}

	_SysDebug("Assigned %p to window %i for %p", WindowPtr, WindowID, Client);	
	Client->Windows[WindowID] = WindowPtr;
}

// --- IPC Message Handlers ---
int IPC_Msg_Ping(tIPC_Client *Client, tAxWin_IPCMessage *Msg)
{
	ASSERT(Msg->ID == IPCMSG_PING);
	
	if( !(Msg->Flags & IPCMSG_FLAG_RETURN) )	return 0;
	
	if( Msg->Size < 4 )	return -1;
	
	tIPCMsg_ReturnInt	*ret = (void*)Msg->Data;
	Msg->ID = IPCMSG_PING;
	Msg->Size = sizeof(*ret);
	ret->Value = AXWIN_VERSION;
	Client->IPCType->SendMessage(Client->Ident, sizeof(*Msg)+sizeof(*ret), Msg);
	return 0;
}

int IPC_Msg_GetDisplayCount(tIPC_Client *Client, tAxWin_IPCMessage *Msg)
{
	tAxWin_IPCMessage	*ret_hdr;
	tIPCMsg_ReturnInt	*ret;
	char	buf[sizeof(*ret_hdr)+sizeof(*ret)];
	
	ASSERT(Msg->ID == IPCMSG_GETDISPLAYCOUNT);

	if( !(Msg->Flags & IPCMSG_FLAG_RETURN) )	return 0;
	
	ret_hdr = (void*)buf;
	ret_hdr->ID = IPCMSG_GETDISPLAYCOUNT;
	ret_hdr->Flags = 0;
	ret_hdr->Window = -1;
	ret_hdr->Size = sizeof(*ret);
	ret = (void*)ret_hdr->Data;
	ret->Value = 1;	// HARD CODE - Current version only supports one display
	
	Client->IPCType->SendMessage(Client->Ident, sizeof(buf), buf);
	return 0;
}

int IPC_Msg_GetDisplayDims(tIPC_Client *Client, tAxWin_IPCMessage *Msg)
{
	tIPCMsg_GetDisplayDims	*info;
	tAxWin_IPCMessage	*ret_hdr;
	tIPCMsg_RetDisplayDims	*ret;
	char	buf[sizeof(*ret_hdr)+sizeof(*ret)];
	
	ASSERT(Msg->ID == IPCMSG_GETDISPLAYDIMS);

	if( !(Msg->Flags & IPCMSG_FLAG_RETURN) )	return 0;

	info = (void*)Msg->Data;	

	ret_hdr = (void*)buf;
	ret_hdr->ID = IPCMSG_GETDISPLAYDIMS;
	ret_hdr->Flags = 0;
	ret_hdr->Window = -1;
	ret_hdr->Size = sizeof(*ret);
	ret = (void*)ret_hdr->Data;
	
	// HARD CODE! Only one display supported
	if( info->DisplayID == 0 )
	{
		ret->X = 0;
		ret->Y = 0;
		ret->W = giScreenWidth;
		ret->H = giScreenHeight;
	}
	else
	{
		ret->X = 0;	ret->Y = 0;
		ret->W = 0;	ret->H = 0;
	}
	
	Client->IPCType->SendMessage(Client->Ident, sizeof(buf), buf);
	return 0;
}

int IPC_Msg_SendMsg(tIPC_Client *Client, tAxWin_IPCMessage *Msg)
{
	tIPCMsg_SendMsg	*info = (void*)Msg->Data;
	tWindow	*src, *dest;

	ASSERT(Msg->ID == IPCMSG_SENDMSG);
	
	// - Sanity checks
	if( Msg->Size < sizeof(tIPCMsg_SendMsg) )
		return -1;
	if( Msg->Size < sizeof(tIPCMsg_SendMsg) + info->Length )
		return -1;
	
	src = IPC_int_GetWindow(Client, Msg->Window);
	if(!src)	return 1;
	dest = IPC_int_GetWindow(Client, info->Remote);
	if(!dest)	return 1;

	WM_SendMessage(src, dest, info->ID, info->Length, info->Data);	

	return 0;
}

int IPC_Msg_CreateWin(tIPC_Client *Client, tAxWin_IPCMessage *Msg)
{
	tIPCMsg_CreateWin	*info = (void*)Msg->Data;
	tWindow	*newwin, *parent;

	ASSERT(Msg->ID == IPCMSG_CREATEWIN);

	// - Sanity checks
	//  > +1 is for NULL byte on string
	if( Msg->Size < sizeof(*info) + 1 ) {
		_SysDebug("IPC_Msg_CreateWin: Size check 1 failed");
		return -1;
	}
	if( info->Renderer[Msg->Size - sizeof(*info) - 1] != '\0' ) {
		_SysDebug("IPC_Msg_CreateWin: Size check 2 failed");
		_SysDebug("info = {");
		_SysDebug("  .NewWinID = %i", info->NewWinID);
		_SysDebug("  .RendererArg = %i", info->RendererArg);
		_SysDebug("  .Renderer = '%.*s'", Msg->Size - sizeof(*info), info->Renderer);
		_SysDebug("}");
		return -1;
	}
	
	// - Get the parent window ID
	parent = IPC_int_GetWindow(Client, Msg->Window);

	// Catch creating a window with an existing ID
	if( IPC_int_GetWindow(Client, info->NewWinID) )
		return 1;

	// - Create the new window, and save its pointer
	newwin = WM_CreateWindow(parent, Client, info->NewWinID, info->RendererArg, info->Renderer);
	IPC_int_SetWindow(Client, info->NewWinID, newwin);

	return 0;
}

int IPC_Msg_DestroyWin(tIPC_Client *Client, tAxWin_IPCMessage *Msg)
{
	tWindow	*win;
	
	ASSERT(Msg->ID == IPCMSG_DESTROYWIN);

	win = IPC_int_GetWindow(Client, Msg->Window);
	if( !win )
		return 0;
	
	WM_DestroyWindow(win);
	IPC_int_SetWindow(Client, Msg->Window, NULL);
	return 0;
}

int IPC_Msg_SetWindowTitle(tIPC_Client *Client, tAxWin_IPCMessage *Msg)
{
	tWindow	*win;

	ASSERT(Msg->ID == IPCMSG_SETWINTITLE);
	
	if( Msg->Size < 1 )	return -1;
	if( Msg->Data[ Msg->Size-1 ] != '\0' )	return -1;	

	win = IPC_int_GetWindow(Client, Msg->Window);
	if(!win)	return 1;

	WM_SetWindowTitle(win, Msg->Data);

	return 0;
}

int IPC_Msg_ShowWindow(tIPC_Client *Client, tAxWin_IPCMessage *Msg)
{
	tIPCMsg_Boolean	*info = (void*)Msg->Data;
	tWindow	*win;
	
	ASSERT(Msg->ID == IPCMSG_SHOWWINDOW);
	
	if( Msg->Size < sizeof(*info) )	return -1;
	
	win = IPC_int_GetWindow(Client, Msg->Window);
	if(!win)	return 1;

	WM_ShowWindow(win, !!info->Value);
	
	return 0;
}

int IPC_Msg_DecorateWindow(tIPC_Client *Client, tAxWin_IPCMessage *Msg)
{
	tIPCMsg_Boolean	*info = (void*)Msg->Data;
	tWindow	*win;
	
	if( Msg->Size < sizeof(*info) )	return -1;
	
	win = IPC_int_GetWindow(Client, Msg->Window);
	if(!win)	return 1;
	
	WM_DecorateWindow(win, !!info->Value);
	return 0;
}

int IPC_Msg_FocusWindow(tIPC_Client *Client, tAxWin_IPCMessage *Msg)
{
	tWindow	*win;

	ASSERT(Msg->ID == IPCMSG_FOCUSWINDOW);
	
	// Don't allow the focus to be changed unless the client has the focus
//	if(!gpWM_FocusedWindow)	return 1;
//	if(gpWM_FocusedWindow->Client != Client)	return 1;

	win = IPC_int_GetWindow(Client, Msg->Window);
	if(!win)	return 1;

	WM_FocusWindow(win);

	return 0;
}

int IPC_Msg_SetWinPos(tIPC_Client *Client, tAxWin_IPCMessage *Msg)
{
	tIPCMsg_SetWindowPos	*info = (void*)Msg->Data;
	tWindow	*win;
	
	ASSERT(Msg->ID == IPCMSG_SETWINPOS);
	
	if(Msg->Size < sizeof(*info))	return -1;
	
	win = IPC_int_GetWindow(Client, Msg->Window);
	if(!win)	return 1;
	
	_SysDebug("info = {..., bSetPos=%i,bSetDims=%i}", info->bSetPos, info->bSetDims);
	
	if(info->bSetPos)
		WM_MoveWindow(win, info->X, info->Y);
	if(info->bSetDims)
		WM_ResizeWindow(win, info->W, info->H);
	
	return 0;
}

int IPC_Msg_RegisterAction(tIPC_Client *Client, tAxWin_IPCMessage *Msg)
{
	tIPCMsg_RegAction	*info = (void*)Msg->Data;
	tWindow	*win;
	
	ASSERT(Msg->ID == IPCMSG_REGACTION);

	if( Msg->Size < sizeof(*info) + 1 )
		return -1;
	
	win = IPC_int_GetWindow(Client, Msg->Window);
	if(!win)	return 1;

	if( strnlen(info->Action, Msg->Size - sizeof(*info)) == Msg->Size - sizeof(*info) )
		return 1;

	_SysDebug("RegisterAction %p:%i [%i]\"%s\"",
		Client, Msg->Window, info->Index, info->Action
		);

	WM_Hotkey_RegisterAction(info->Action, win, info->Index);

	return 0;
}

int (*gIPC_MessageHandlers[])(tIPC_Client *Client, tAxWin_IPCMessage *Msg) = {
	IPC_Msg_Ping,
	IPC_Msg_GetDisplayCount,
	IPC_Msg_GetDisplayDims,
	IPC_Msg_SendMsg,
	IPC_Msg_CreateWin,
	IPC_Msg_DestroyWin,	// Destroy window
	IPC_Msg_SetWindowTitle,
	IPC_Msg_ShowWindow,
	IPC_Msg_DecorateWindow,
	IPC_Msg_FocusWindow,
	IPC_Msg_SetWinPos,
	IPC_Msg_RegisterAction
};
const int giIPC_NumMessageHandlers = sizeof(gIPC_MessageHandlers)/sizeof(gIPC_MessageHandlers[0]);

void IPC_Handle(tIPC_Client *Client, size_t MsgLen, tAxWin_IPCMessage *Msg)
{
	 int	rv = 0;
	
//	_SysDebug("IPC_Handle: (IPCType=%p, Ident=%p, MsgLen=%i, Msg=%p)",
//		IPCType, Ident, MsgLen, Msg);
	
	if( MsgLen < sizeof(tAxWin_IPCMessage) )
		return ;
	if( MsgLen < sizeof(tAxWin_IPCMessage) + Msg->Size )
		return ;
	
	if( Msg->Flags & IPCMSG_FLAG_RENDERER )
	{
		tWindow *win = IPC_int_GetWindow(Client, Msg->Window);
		if( !win ) {
			_SysDebug("WARNING: NULL window in message %i (%x)", Msg->ID, Msg->Window);
			return ;
		}
		tWMRenderer	*renderer = win->Renderer;
		if( Msg->ID >= renderer->nIPCHandlers ) {
			_SysDebug("WARNING: Message %i out of range in %s", Msg->ID, renderer->Name);
			return ;
		}
		if( !renderer->IPCHandlers[Msg->ID] ) {
			_SysDebug("WARNING: Message %i has no handler in %s", Msg->ID, renderer->Name);
			return ;
		}
		_SysDebug("IPC_Handle: Call %s-%i", renderer->Name, Msg->ID);
		rv = renderer->IPCHandlers[Msg->ID](win, Msg->Size, Msg->Data);
		if( rv )
			_SysDebug("IPC_Handle: rv != 0 (%i)", rv);
	}
	else
	{
		if( Msg->ID >= giIPC_NumMessageHandlers ) {
			fprintf(stderr, "WARNING: Unknown message %i (%p)\n", Msg->ID, Client);
			_SysDebug("WARNING: Unknown message %i (%p)", Msg->ID, Client);
			return ;
		}
		
		if( !gIPC_MessageHandlers[ Msg->ID ] ) {
			fprintf(stderr, "WARNING: Message %i does not have a handler\n", Msg->ID);
			_SysDebug("WARNING: Message %i does not have a handler", Msg->ID);
			return ;
		}
	
		_SysDebug("IPC_Handle: Call WM-%i", Msg->ID);
		rv = gIPC_MessageHandlers[Msg->ID](Client, Msg);
		if( rv )
			_SysDebug("IPC_Handle: rv != 0 (%i)", rv);
	}
}

// Dispatch a message to the client
void IPC_SendWMMessage(tIPC_Client *Client, uint32_t Src, uint32_t Dst, int MsgID, int Len, void *Data)
{
	tAxWin_IPCMessage	*hdr;
	tIPCMsg_SendMsg 	*msg;
	char	buf[sizeof(*hdr)+sizeof(*msg)+Len];
	
	hdr = (void*)buf;
	msg = (void*)hdr->Data;
	
	hdr->ID = IPCMSG_SENDMSG;
	hdr->Flags = 0;
	hdr->Size = sizeof(*msg) + Len;
	hdr->Window = Dst;
	
	msg->Remote = Src;
	msg->ID = MsgID;
	msg->Length = Len;
	memcpy(msg->Data, Data, Len);
	
	Client->IPCType->SendMessage(Client->Ident, sizeof(buf), buf);
}

// --- Server->Client replies
void IPC_SendReply(tIPC_Client *Client, uint32_t WinID, int MsgID, size_t Len, const void *Data)
{
	tAxWin_IPCMessage	*hdr;
	char	buf[sizeof(*hdr)+Len];
	
	hdr = (void*)buf;
	
	hdr->ID = MsgID;
	hdr->Flags = IPCMSG_FLAG_RENDERER;
	hdr->Size = Len;
	hdr->Window = WinID;
	
	memcpy(hdr->Data, Data, Len);
	Client->IPCType->SendMessage(Client->Ident, sizeof(buf), buf);
}

