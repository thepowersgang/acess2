/*
 * AxWin3 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * wm.c
 * - Core window management functions
 */
#include <axwin3/axwin.h>
#include <stdlib.h>
#include <string.h>
#include "include/internal.h"
#include "include/ipc.h"
#include <wm_messages.h>

#define WINDOWS_PER_ALLOC	(64-1)	// -1 to make it 64 pointers (+ Next)
#define MAX_HOTKEYS	32

typedef struct sWindowBlock	tWindowBlock;

typedef struct sAxWin3_Window	tWindow;

// === STRUCTURES ===
struct sWindowBlock
{
	tWindowBlock	*Next;
	tWindow	*Windows[WINDOWS_PER_ALLOC];
};

// === GLOBALS ===
 int	giAxWin3_LowestFreeWinID;
 int	giAxWin3_HighestUsedWinID;
tWindowBlock	gAxWin3_WindowList;
tAxWin3_HotkeyCallback	gAxWin3_Hotkeys[MAX_HOTKEYS];

// === CODE ===
tWindow *AxWin3_int_CreateWindowStruct(uint32_t ServerID, int ExtraBytes)
{
	tWindow	*ret;
	
	ret = calloc(sizeof(tWindow) + ExtraBytes, 1);
	ret->ServerID = ServerID;

	return ret;
}

tWindow *AxWin3_int_GetWindowFromID(uint32_t ServerID)
{
	tWindowBlock	*block = &gAxWin3_WindowList;
	while(block && ServerID > WINDOWS_PER_ALLOC) {
		block = block->Next;
		ServerID -= WINDOWS_PER_ALLOC;
	}
	if(!block)	return NULL;
	return block->Windows[ServerID];
}

void AxWin3_int_DelWindowByID(uint32_t ServerID)
{
	 int	orig_id = ServerID;
	tWindowBlock	*block = &gAxWin3_WindowList;
	while(block && ServerID > WINDOWS_PER_ALLOC) {
		block = block->Next;
		ServerID -= WINDOWS_PER_ALLOC;
	}
	if( !block || !block->Windows[ServerID] )
		_SysDebug("AxWin3_int_DelWindowByID - Id %i out of range", orig_id);
	else
		block->Windows[ServerID] = NULL;
}

tWindow *AxWin3_int_AllocateWindowInfo(int DataBytes, int *WinID)
{
	 int	idx, newWinID;
	tWindowBlock *block, *prev = NULL;
	tWindow	*ret;	

	block = &gAxWin3_WindowList;
	newWinID = giAxWin3_LowestFreeWinID;
	for( idx = 0; block; newWinID ++ )
	{
		if( block->Windows[idx] == NULL )
			break;
		idx ++;
		if(idx == WINDOWS_PER_ALLOC) {
			prev = block;
			block = block->Next;
			idx = 0;
		}
	}
	
	if( !block )
	{
		block = calloc(sizeof(tWindowBlock), 1);
		prev->Next = block;
		idx = 0;
	}
	
	ret = block->Windows[idx] = AxWin3_int_CreateWindowStruct(newWinID, DataBytes);
	if(newWinID > giAxWin3_HighestUsedWinID)
		giAxWin3_HighestUsedWinID = newWinID;
	if(newWinID == giAxWin3_LowestFreeWinID)
		giAxWin3_HighestUsedWinID ++;

	if(WinID)	*WinID = newWinID;

	return ret;
}

int AxWin3_GetDisplayCount(void)
{
	tAxWin_IPCMessage	*msg;
	tIPCMsg_ReturnInt	*ret;
	 int	rv;
	
	msg = AxWin3_int_AllocateIPCMessage(NULL, IPCMSG_GETDISPLAYCOUNT, IPCMSG_FLAG_RETURN, 0);
	AxWin3_int_SendIPCMessage(msg);
	free(msg);	

	msg = AxWin3_int_WaitIPCMessage(IPCMSG_GETDISPLAYCOUNT);
	if(msg->Size < sizeof(*ret))	return -1;
	ret = (void*)msg->Data;
	rv = ret->Value;
	free(msg);
	return rv;
}

int AxWin3_GetDisplayDims(int Display, int *X, int *Y, int *Width, int *Height)
{
	tAxWin_IPCMessage	*msg;
	tIPCMsg_GetDisplayDims	*req;
	tIPCMsg_RetDisplayDims	*ret;
	
	msg = AxWin3_int_AllocateIPCMessage(NULL, IPCMSG_GETDISPLAYDIMS, IPCMSG_FLAG_RETURN, sizeof(*req));
	req = (void*)msg->Data;
	req->DisplayID = Display;
	AxWin3_int_SendIPCMessage(msg);
	free(msg);

	msg = AxWin3_int_WaitIPCMessage(IPCMSG_GETDISPLAYDIMS);
	if(msg->Size < sizeof(*ret))	return -1;
	ret = (void*)msg->Data;
	
	if(X)	*X = ret->X;
	if(Y)	*Y = ret->Y;
	if(Width)	*Width = ret->W;
	if(Height)	*Height = ret->H;
	
	return 0;
}

tHWND AxWin3_CreateWindow(
	tHWND Parent, const char *Renderer, int RendererArg,
	int DataBytes, tAxWin3_WindowMessageHandler MessageHandler
	)
{
	tWindow	*ret;
	 int	newWinID;
	 int	dataSize = sizeof(tIPCMsg_CreateWin) + strlen(Renderer) + 1;
	tAxWin_IPCMessage	*msg;
	tIPCMsg_CreateWin	*create_win;

	// Allocate a window ID
	ret = AxWin3_int_AllocateWindowInfo(DataBytes, &newWinID);
	if(!ret)	return NULL;
	ret->Handler = MessageHandler;

	// Create message
	msg = AxWin3_int_AllocateIPCMessage(Parent, IPCMSG_CREATEWIN, 0, dataSize);
	create_win = (void*)msg->Data;	
	create_win->NewWinID = newWinID;
	create_win->RendererArg = RendererArg;
	strcpy(create_win->Renderer, Renderer);

	// Send and clean up
	AxWin3_int_SendIPCMessage(msg);
	free(msg);

	_SysDebug("AxWin3_CreateWindow: %i :: '%s'", newWinID, Renderer);

	// TODO: Detect and handle possible errors

	// Return success
	return ret;
}

void AxWin3_DestroyWindow(tHWND Window)
{
	tAxWin_IPCMessage	*msg;
	
	msg = AxWin3_int_AllocateIPCMessage(Window, IPCMSG_DESTROYWIN, 0, 0);
	AxWin3_int_SendIPCMessage(msg);
	free(msg);
}

void *AxWin3_int_GetDataPtr(tHWND Window)
{
	return Window->Data;
}

int AxWin3_int_DefaultMessageHandler(tWindow *Win, int ID, size_t Len, const void *Data)
{
	switch(ID)
	{
	case WNDMSG_HOTKEY: {
		const struct sWndMsg_Hotkey *mi = Data;
		if( Len < sizeof(*mi) )
			return -1;

		if( mi->ID >= MAX_HOTKEYS ) 
			_SysDebug("--- Out of range hotkey %i fired", mi->ID);
		else if( gAxWin3_Hotkeys[mi->ID] == 0 )
			_SysDebug("--- Unmapped hotkey ID %i fired", mi->ID);
		else
			gAxWin3_Hotkeys[mi->ID]();
		}
		return 1;
	// Honour a close message by default
	case WNDMSG_CLOSE:
		AxWin3_DestroyWindow(Win);
		return 1;
	// Zero fucks given?
	case WNDMSG_DESTROY:
		_SysDebug("TODO: Check that WNDMSG_DESTROY was from us calling _DestroyWindow");
		// TODO: Finalise cleanup of window, this will be the last message sent to this window
		AxWin3_int_DelWindowByID(Win->ServerID);
		return 1;
	default:
		return 0;
	}
}

void AxWin3_int_HandleMessage(tAxWin_IPCMessage *Msg)
{
	tWindow	*dest;

	dest = AxWin3_int_GetWindowFromID(Msg->Window);

	switch(Msg->ID)
	{
	case IPCMSG_SENDMSG: {
		tIPCMsg_SendMsg	*info = (void*)Msg->Data;
		if(Msg->Size < sizeof(*info) || Msg->Size < sizeof(*info) + info->Length) {
			_SysDebug("Message is undersized (%i < %i + %i)",
				Msg->Size < sizeof(*info), info->Length);
			return ;
		}
		if(!dest || !dest->Handler) {
			_SysDebug("No handler for destination %p", dest);
			return ;
		}
		_SysDebug("IPC Message 0x%x - %i bytes", info->ID, info->Length);

		if( dest->Handler(dest, info->ID, info->Length, info->Data) )
			;
		else if( AxWin3_int_DefaultMessageHandler(dest, info->ID, info->Length, info->Data) )
			;
		else
			_SysDebug("--- Unhandled SENDMSG 0x%x win %i", info->ID, Msg->Window);
		break; }
	case IPCMSG_DESTROYWIN:
		// Clean up resources associated with this window
		break;
	default:
		_SysDebug("Unknow message ID %i", Msg->ID);
		break;
	}
	
	free(Msg);
}

void AxWin3_SetWindowTitle(tHWND Window, const char *Title)
{
	tAxWin_IPCMessage	*msg;
	 int	len = strlen(Title);
	
	msg = AxWin3_int_AllocateIPCMessage(Window, IPCMSG_SETWINTITLE, 0, len+1);
	strcpy(msg->Data, Title);
	
	AxWin3_int_SendIPCMessage(msg);
	
	free(msg);
}

void AxWin3_SendMessage(tHWND Window, tHWND Destination, int Message, int Length, void *Data)
{
	tAxWin_IPCMessage	*msg;
	tIPCMsg_SendMsg	*info;
	
	msg = AxWin3_int_AllocateIPCMessage(Window, IPCMSG_SENDMSG, 0, sizeof(*info)+Length);
	info = (void*)msg->Data;
	info->Remote = AxWin3_int_GetWindowID(Destination);
	info->ID = Message;
	info->Length = Length;
	memcpy(info->Data, Data, Length);
	
	AxWin3_int_SendIPCMessage(msg);
	free(msg);
}

void *AxWin3_WaitMessage(tHWND Window, int MessageID, size_t *Length)
{
	tAxWin_IPCMessage	*msg;
	
	for( ;; )
	{
		msg = AxWin3_int_WaitIPCMessage(IPCMSG_SENDMSG);
		if( msg->Window != AxWin3_int_GetWindowID(Window) ) {
			AxWin3_int_HandleMessage(msg);
			continue ;
		}
		tIPCMsg_SendMsg	*info = (void*)msg->Data;
		if( info->ID != MessageID ) {
			AxWin3_int_HandleMessage(msg);
			continue ;
		}

		*Length = info->Length;
		void	*ret = malloc(info->Length);	
		memcpy(ret, info->Data, info->Length);
		free(msg);
	
		return ret;
	}
}

void AxWin3_SendIPC(tHWND Window, int Message, size_t Length, const void *Data)
{
	tAxWin_IPCMessage	*msg;
	
	msg = AxWin3_int_AllocateIPCMessage(Window, Message, IPCMSG_FLAG_RENDERER, Length);
	memcpy(msg->Data, Data, Length);
	AxWin3_int_SendIPCMessage(msg);
	free(msg);
}

void *AxWin3_WaitIPCMessage(tHWND Window, int MessageID, size_t *Length)
{
	tAxWin_IPCMessage	*msg;
	for( ;; )
	{
		msg = AxWin3_int_WaitIPCMessage(MessageID);
		if( !(msg->Flags & IPCMSG_FLAG_RENDERER) || msg->Window != AxWin3_int_GetWindowID(Window) ) {
			AxWin3_int_HandleMessage(msg);
			continue ;
		}

		*Length = msg->Size;
		void	*ret = malloc(msg->Size);
		memcpy(ret, msg->Data, msg->Size);
		free(msg);
	
		return ret;
	}
}

void AxWin3_FocusWindow(tHWND Window)
{
	tAxWin_IPCMessage	*msg;
	
	msg = AxWin3_int_AllocateIPCMessage(Window, IPCMSG_FOCUSWINDOW, 0, 0);
	
	AxWin3_int_SendIPCMessage(msg);
	free(msg);
}

void AxWin3_ShowWindow(tHWND Window, int bShow)
{
	tAxWin_IPCMessage	*msg;
	tIPCMsg_Boolean	*info;

	msg = AxWin3_int_AllocateIPCMessage(Window, IPCMSG_SHOWWINDOW, 0, sizeof(*info));
	info = (void*)msg->Data;
	info->Value = !!bShow;
	
	AxWin3_int_SendIPCMessage(msg);
	
	free(msg);
}

void AxWin3_DecorateWindow(tHWND Window, int bDecorate)
{
	tAxWin_IPCMessage	*msg;
	tIPCMsg_Boolean	*info;

	msg = AxWin3_int_AllocateIPCMessage(Window, IPCMSG_DECORATEWINDOW, 0, sizeof(*info));
	info = (void*)msg->Data;
	info->Value = !!bDecorate;
	
	AxWin3_int_SendIPCMessage(msg);
	
	free(msg);
}

void AxWin3_SetWindowPos(tHWND Window, short X, short Y, short W, short H)
{
	tAxWin_IPCMessage	*msg;
	tIPCMsg_SetWindowPos	*info;
	
	msg = AxWin3_int_AllocateIPCMessage(Window, IPCMSG_SETWINPOS, 0, sizeof(*info));
	info = (void*)msg->Data;

	info->bSetPos = 1;
	info->bSetDims = 1;
	info->X = X;	info->Y = Y;
	info->W = W;	info->H = H;

	AxWin3_int_SendIPCMessage(msg);	
	free(msg);
}

void AxWin3_MoveWindow(tHWND Window, short X, short Y)
{
	tAxWin_IPCMessage	*msg;
	tIPCMsg_SetWindowPos	*info;
	
	msg = AxWin3_int_AllocateIPCMessage(Window, IPCMSG_SETWINPOS, 0, sizeof(*info));
	info = (void*)msg->Data;

	info->bSetPos = 1;
	info->bSetDims = 0;
	info->X = X;
	info->Y = Y;
	
	AxWin3_int_SendIPCMessage(msg);
	
	free(msg);
}

void AxWin3_ResizeWindow(tHWND Window, short W, short H)
{
	tAxWin_IPCMessage	*msg;
	tIPCMsg_SetWindowPos	*info;
	
	msg = AxWin3_int_AllocateIPCMessage(Window, IPCMSG_SETWINPOS, 0, sizeof(*info));
	info = (void*)msg->Data;

	info->bSetPos = 0;
	info->bSetDims = 1;
	info->W = W;
	info->H = H;
	
	AxWin3_int_SendIPCMessage(msg);
	
	free(msg);
}

int AxWin3_RegisterAction(tHWND Window, const char *Action, tAxWin3_HotkeyCallback cb)
{
	 int	i;
	for( i = 0; i < MAX_HOTKEYS; i ++ )
	{
		if( gAxWin3_Hotkeys[i] == NULL )
		{
			tAxWin_IPCMessage	*msg;
			struct sIPCMsg_RegAction	*info;
			gAxWin3_Hotkeys[i] = cb;
			
			msg = AxWin3_int_AllocateIPCMessage(Window, IPCMSG_REGACTION, 0, sizeof(*info)+strlen(Action)+1);
			info = (void*)msg->Data;
		
			info->Index = i;
			strcpy(info->Action, Action);
			
			AxWin3_int_SendIPCMessage(msg);
			free(msg);
			return i;
		}
	}
	return -1;
}

