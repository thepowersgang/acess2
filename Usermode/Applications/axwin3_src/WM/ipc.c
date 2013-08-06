/*
 * Acess2 GUI (AxWin) Version 3
 * - By John Hodge (thePowersGang)
 * 
 * ipc.c
 * - Interprocess communication
 */
#include <common.h>
#include <string.h>
#include <ipcmessages.h>
#include <stdio.h>
#include <wm.h>
#include <wm_internals.h>
#include <wm_hotkeys.h>	// Hotkey registration
#include <wm_renderer.h>	// Renderer IPC messages
#include <ipc_int.h>

#define MAX_WINDOWS_PER_APP	128

// === IMPORTS ===
extern tWindow	*gpWM_FocusedWindow;	// Needed for _FocusWindow

// === PROTOTYPES ===
tIPC_Client	*IPC_int_GetClient(const tIPC_Type *IPCType, const void *Ident);
void	IPC_int_DropClient(tIPC_Client *Client);
void	IPC_Handle(tIPC_Client *Client, size_t MsgLen, tAxWin_IPCMessage *Msg);

// === GLOBALS ===
 int	giIPC_ClientCount;
tIPC_Client	**gIPC_Clients;

// === CODE ===
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
		_SysDebug("IPC_Handle: Call %s-%i %ib", renderer->Name, Msg->ID, Msg->Size);
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
	
		_SysDebug("IPC_Handle: Call WM-%i %ib", Msg->ID, Msg->Size);
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

