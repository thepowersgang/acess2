/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * wm.c
 * - Window manager core
 */
#include <common.h>
#include <wm_renderer.h>
#include <stdlib.h>
#include <string.h>
#include <video.h>
#include <wm_messages.h>
#include <decorator.h>
#include <axwin3/keysyms.h>

// === IMPORTS ===
extern int	Renderer_Menu_Init(void);
extern int	Renderer_Widget_Init(void);
extern int	Renderer_Background_Init(void);
extern int	Renderer_Framebuffer_Init(void);
extern int	Renderer_RichText_Init(void);

extern void	IPC_SendWMMessage(tIPC_Client *Client, uint32_t Src, uint32_t Dst, int Msg, int Len, const void *Data);
extern void	IPC_SendReply(tIPC_Client *Client, uint32_t WinID, int MsgID, size_t Len, const void *Data);
extern tWindow	*IPC_int_GetWindow(tIPC_Client *Client, uint32_t ID);

// === GLOBALS ===
tWMRenderer	*gpWM_Renderers;
tWindow	*gpWM_RootWindow;
//! Window which will recieve the next keyboard event
tWindow	*gpWM_FocusedWindow;
//! Hilighted window (owner of the currently focused window)
tWindow	*gpWM_HilightedWindow;

// === CODE ===
void WM_Initialise(void)
{
	// TODO: Autodetect these
	Renderer_Menu_Init();
	Renderer_Widget_Init();
	Renderer_Background_Init();
	Renderer_Framebuffer_Init();
	Renderer_RichText_Init();
	
	WM_CreateWindow(NULL, NULL, 0, 0x0088FF, "Background");
	gpWM_RootWindow->W = giScreenWidth;
	gpWM_RootWindow->H = giScreenHeight;
	gpWM_RootWindow->Flags = WINFLAG_SHOW|WINFLAG_NODECORATE;

	// TODO: Move these to config
	uint32_t	keys[4];
	keys[0] = KEYSYM_LEFTGUI;	keys[1] = KEYSYM_r;
	WM_Hotkey_Register(2, keys, "Interface>Run");
	keys[0] = KEYSYM_LEFTGUI;	keys[1] = KEYSYM_t;
	WM_Hotkey_Register(2, keys, "Interface>Terminal");
	keys[0] = KEYSYM_LEFTGUI;	keys[1] = KEYSYM_e;
	WM_Hotkey_Register(2, keys, "Interface>TextEdit");
}

void WM_RegisterRenderer(tWMRenderer *Renderer)
{
	// Catch out duplicates
	for(tWMRenderer *r = gpWM_Renderers; r; r = r->Next ) {
		if( r == Renderer ) {
			return ;
		}
		if( strcmp(r->Name, Renderer->Name) == 0 ) {
			return ;
		}
	}
	
	Renderer->Next = gpWM_Renderers;
	gpWM_Renderers = Renderer;
}

// --- Manipulation
tWindow *WM_CreateWindow(tWindow *Parent, tIPC_Client *Client, uint32_t ID, int RendererArg, const char *RendererName)
{
	tWMRenderer	*renderer;
	tWindow	*ret;
	
	// - Get Renderer
	for( renderer = gpWM_Renderers; renderer; renderer = renderer->Next )
	{
		if(strcmp(RendererName, renderer->Name) == 0)
			break;
	}
	if(renderer == NULL)
		return NULL;

	// - Call create window function
	ret = renderer->CreateWindow(RendererArg);
	ret->Client = Client;
	ret->ID = ID;
	ret->Parent = Parent;
	if(!ret->Parent)
		ret->Parent = gpWM_RootWindow;
	ret->Owner = Parent;
	ret->Renderer = renderer;
	ret->Flags |= WINFLAG_CLEAN;	// Needed to stop invaildate early exiting

	// Append to parent
	if(ret->Parent)
	{
		if(ret->Parent->LastChild)
			ret->Parent->LastChild->NextSibling = ret;
		else
			ret->Parent->FirstChild = ret;
		ret->PrevSibling = ret->Parent->LastChild;
		ret->Parent->LastChild = ret;
		ret->NextSibling = NULL;
	}
	else
	{
		gpWM_RootWindow = ret;
	}

	// Don't decorate child windows by default
	if(Parent)
	{
		ret->Flags |= WINFLAG_NODECORATE;
	}
	
	// - Return!
	return ret;
}

void WM_DestroyWindow(tWindow *Window)
{
	// TODO: Lock window and flag as invalid
	
	// - Remove from render tree
	{
		// TODO: Lock render tree?
		tWindow *prev = Window->PrevSibling;
		tWindow *next = Window->NextSibling;
		if(prev)
			prev->NextSibling = next;
		else
			Window->Parent->FirstChild = next;
		if(next)
			next->PrevSibling = prev;
		else
			Window->Parent->LastChild = prev;
	}
	// - Full invalidate
	WM_Invalidate(Window, 1);
	
	// - Remove from inheritance tree?
	
	// - Clean up render children
	{
		// Lock should not be needed
		tWindow *win, *next;
		for( win = Window->FirstChild; win; win = next )
		{
			next = win->NextSibling;
			ASSERT(Window->FirstChild->Parent == Window);
			WM_DestroyWindow(win);
		}
	}
	
	// - Clean up inheriting children?

	// - Tell renderer to clean up
	if( Window->Renderer->DestroyWindow )
		Window->Renderer->DestroyWindow(Window);
	else
		_SysDebug("WARN: Renderer '%s' does not have a destroy function", Window->Renderer->Name);

	// - Tell client to clean up
	WM_SendMessage(NULL, Window, WNDMSG_DESTROY, 0, NULL);

	// - Clean up render cache and window structure
	free(Window->Title);
	free(Window->RenderBuffer);
	free(Window);
}

tWindow *WM_GetWindowByID(tWindow *Requester, uint32_t ID)
{
	return IPC_int_GetWindow(Requester->Client, ID);
}

tWindow *WM_CreateWindowStruct(size_t ExtraSize)
{
	tWindow	*ret;
	
	ret = calloc( sizeof(tWindow) + ExtraSize, 1 );
	ret->RendererInfo = ret + 1;	// Get end of tWindow
	
	return ret;
}

void WM_SetWindowTitle(tWindow *Window, const char *Title)
{
	if(Window->Title)
		free(Window->Title);
	Window->Title = strdup(Title);
	_SysDebug("Window %p title set to '%s'", Window, Title);
}

void WM_RaiseWindow(tWindow *Window)
{
	tWindow	*parent = Window->Parent;
	if(!Window->Parent)	return ;
	
	// Remove from list
	if(Window->PrevSibling)
		Window->PrevSibling->NextSibling = Window->NextSibling;
	if(Window->NextSibling)
		Window->NextSibling->PrevSibling = Window->PrevSibling;
	if(parent->FirstChild == Window)
		parent->FirstChild = Window->NextSibling;
	if(parent->LastChild == Window)
		parent->LastChild = Window->PrevSibling;

	// Append to end
	if(parent->LastChild)
		parent->LastChild->NextSibling = Window;
	else
		parent->FirstChild = Window;
	Window->PrevSibling = parent->LastChild;
	Window->NextSibling = NULL;
	parent->LastChild = Window;
	
	_SysDebug("Raised %p", Window);
}

/*
void WM_RaiseWindow(tWindow *Window)
{
	// Move to the last render position (move to top)
	while(Window && Window->Parent)
	{
		if( Window->NextSibling )
		{
			// remove
			if( Window->PrevSibling )
				Window->PrevSibling->NextSibling = Window->NextSibling;
			Window->NextSibling->PrevSibling = Window->PrevSibling;
			// Mutate self
			Window->PrevSibling = Window->Parent->LastChild;
			Window->NextSibling = NULL;
			// re-add
			Window->PrevSibling->NextSibling = Window;
			Window->Parent->LastChild = Window;
		}
		_SysDebug("WM_RaiseWindow: Raised %p", Window);
		Window = Window->Parent;
	}
}
*/

void WM_FocusWindow(tWindow *Destination)
{
	struct sWndMsg_Bool	_msg;
	
	_SysDebug("WM_FocusWindow(%p)", Destination);

	if( gpWM_FocusedWindow == Destination )
		return ;
	if( Destination && !(Destination->Flags & WINFLAG_SHOW) )
		return ;
	
	if( gpWM_FocusedWindow )
	{
		_msg.Val = 0;
		WM_SendMessage(NULL, gpWM_FocusedWindow, WNDMSG_FOCUS, sizeof(_msg), &_msg);
	}
	if( Destination )
	{
		_msg.Val = 1;
		WM_SendMessage(NULL, Destination, WNDMSG_FOCUS, sizeof(_msg), &_msg);
	}
	
	// TODO: Leave it up to the renderer to decide to invalidate
	WM_Invalidate(gpWM_FocusedWindow, 1);
	WM_Invalidate(Destination, 1);

	gpWM_FocusedWindow = Destination;
}


void WM_ShowWindow(tWindow *Window, int bShow)
{
	struct sWndMsg_Bool	_msg;
	
	if( !!(Window->Flags & WINFLAG_SHOW) == bShow )
		return ;
	
	// Window is being hidden, invalidate parents
	if( !bShow )
		WM_Invalidate(Window, 0);

	// Message window
	_msg.Val = !!bShow;
	WM_SendMessage(NULL, Window, WNDMSG_SHOW, sizeof(_msg), &_msg);

	// Update the flag
	if(bShow) {
		Window->Flags |= WINFLAG_SHOW;
		_SysDebug("Window %p shown", Window);
	}
	else
	{
		Window->Flags &= ~WINFLAG_SHOW;

		if( Window == gpWM_FocusedWindow )
			WM_FocusWindow(Window->Parent);
		
		// Just a little memory saving for large hidden windows
		if(Window->RenderBuffer) {
			free(Window->RenderBuffer);
			Window->RenderBuffer = NULL;
		}
		_SysDebug("Window %p hidden", Window);
	}
	
	// Window has been shown, invalidate everything
	if( bShow )
		WM_Invalidate(Window, 1);
}

void WM_DecorateWindow(tWindow *Window, int bDecorate)
{
	if( !(Window->Flags & WINFLAG_NODECORATE) == !!bDecorate )
		return ;
	
	if(bDecorate)
		Window->Flags &= ~WINFLAG_NODECORATE;
	else
		Window->Flags |= WINFLAG_NODECORATE;
	
	// Needed because the window size changes
	if(Window->RenderBuffer) {
		free(Window->RenderBuffer);
		Window->RenderBuffer = NULL;
	}
	
	WM_Invalidate(Window, 1);
}

void WM_SetRelative(tWindow *Window, int bRelativeToParent)
{
//	_SysDebug("WM_SetRelative: (%p{Parent=%p},%i)", Window, Window->Parent, bRelativeToParent);
	// No meaning if no parent
	if( !Window->Parent )
		return ;

	// Check that the flag is changing
	if( !!bRelativeToParent == !!(Window->Flags & WINFLAG_RELATIVE) )
		return ;

	if( bRelativeToParent ) {
		// Set
		Window->Flags |= WINFLAG_RELATIVE;
		WM_MoveWindow(Window, Window->X, Window->Y);
	}
	else {
		// Clear
		Window->Flags &= ~WINFLAG_RELATIVE;
		WM_MoveWindow(Window, Window->X - Window->Parent->X, Window->Y - Window->Parent->Y);
	}
}

int WM_MoveWindow(tWindow *Window, int X, int Y)
{
//	_SysDebug("Move %p to (%i,%i)", Window, X, Y);
	// Clip coordinates
	if(X + Window->W < 0)	X = -Window->W + 1;
	if(Y + Window->H < 0)	Y = -Window->H + 1;
	if(X >= giScreenWidth)	X = giScreenWidth - 1;
	if(Y >= giScreenHeight)	Y = giScreenHeight - 1;
	
	// If relative to the parent, extra checks
	if( (Window->Flags & WINFLAG_RELATIVE) && Window->Parent )
	{
		if( X > Window->Parent->W )	return 1;
		if( Y > Window->Parent->H )	return 1;
	}
	// TODO: Re-sanitise

	if( Window->X == X && Window->Y == Y ) {
		_SysDebug("WM_MoveWindow: Equal (%i,%i)", X, Y);
		return 0;
	}

	if( Window->Parent )
		Window->Parent->Flags |= WINFLAG_NEEDREBLIT;

	_SysDebug("WM_MoveWindow: (%i,%i)", X, Y);
	Window->X = X;	Window->Y = Y;

	// Mark up the tree that a child window has changed	
	WM_Invalidate(Window, 0);

	return 0;
}

int WM_ResizeWindow(tWindow *Window, int W, int H)
{
	if(W <= 0 || H <= 0 )	return 1;
	if(Window->X + W < 0)	Window->X = -W + 1;
	if(Window->Y + H < 0)	Window->Y = -H + 1;

	if( Window->W == W && Window->H == H )
		return 0;
	
	// If the window size has decreased, force the parent to reblit
	if( Window->Parent && (Window->W > W || Window->H > H) )
		Window->Parent->Flags |= WINFLAG_NEEDREBLIT;

	_SysDebug("WM_ResizeWindow: %p:%i %ix%i", Window->Client, Window->ID, W, H);
	Window->W = W;	Window->H = H;

	if(Window->RenderBuffer) {
		free(Window->RenderBuffer);
		Window->RenderBuffer = NULL;
	}
	WM_Invalidate(Window, 1);

	{
		struct sWndMsg_Resize	msg;
		msg.W = W;
		msg.H = H;
		WM_SendMessage(NULL, Window, WNDMSG_RESIZE, sizeof(msg), &msg);
	}
	
	return 0;
}

int WM_SendMessage(tWindow *Source, tWindow *Dest, int Message, int Length, const void *Data)
{
//	_SysDebug("WM_SendMessage: (%p, %p, %i, %i, %p)", Source, Dest, Message, Length, Data);
	if(Dest == NULL) {
		_SysDebug("WM_SendMessage: NULL destination from %p", __builtin_return_address(0));
		return -2;
	}
	if(Length > 0 && Data == NULL) {
		_SysDebug("WM_SendMessage: non-zero length and NULL data");
		return -1;
	}

	if( Decorator_HandleMessage(Dest, Message, Length, Data) != 1 )
	{
		// TODO: Catch errors from ->HandleMessage
//		_SysDebug("WM_SendMessage: Decorator grabbed message?");
		return 0;
	}
	
	// ->HandleMessage returns 1 when the message was not handled
	if( Dest->Renderer->HandleMessage(Dest, Message, Length, Data) != 1 )
	{
		// TODO: Catch errors from ->HandleMessage
//		_SysDebug("WM_SendMessage: Renderer grabbed message?");
		return 0;
	}

	// TODO: Implement message masking

	// Dispatch to client
	if(Dest->Client)
	{
		uint32_t	src_id;
		if(!Source)
			src_id = -1;
		else if(Source->Client != Dest->Client) {
			// TODO: Support different client source windows
			_SysDebug("WM_SendMessage: TODO - Support inter-client messages");
			return -1;
		}
		else {
			src_id = Source->ID;
		}
		
//		_SysDebug("WM_SendMessage: IPC Dispatch");
		IPC_SendWMMessage(Dest->Client, src_id, Dest->ID, Message, Length, Data);
	}	

	return 1;
}

int WM_SendIPCReply(tWindow *Window, int Message, size_t Length, const void *Data)
{
	IPC_SendReply(Window->Client, Window->ID, Message, Length, Data);
	return 0;
}

void WM_Invalidate(tWindow *Window, int bClearClean)
{
	if(!Window)	return ;
	
	// Don't bother invalidating if the window isn't shown
	if( !(Window->Flags & WINFLAG_SHOW) )
		return ;
	
	// Mark for re-render
	if( bClearClean )
		Window->Flags &= ~WINFLAG_CLEAN;

	// Mark up the tree that a child window has changed	
	while( (Window = Window->Parent) && (Window->Flags & WINFLAG_SHOW) )
		Window->Flags &= ~WINFLAG_CHILDCLEAN;
}

// --- Rendering / Update
void WM_int_UpdateWindow(tWindow *Window)
{
	 int	bDecoratorRedraw = 0;

	// Ignore hidden windows
	if( !(Window->Flags & WINFLAG_SHOW) )
		return ;
	
	if( (Window->Flags & WINFLAG_RELATIVE) && Window->Parent )
	{
		Window->RealX = Window->Parent->RealX + Window->Parent->BorderL + Window->X;
		Window->RealY = Window->Parent->RealY + Window->Parent->BorderT + Window->Y;
	}
	else
	{
		Window->RealX = Window->X;
		Window->RealY = Window->Y;
	}
	
	// Render
	if( !(Window->Flags & WINFLAG_CLEAN) )
	{
		// Calculate RealW/RealH
		if( !(Window->Flags & WINFLAG_NODECORATE) )
		{
			//_SysDebug("Applying decorations to %p", Window);
			Decorator_UpdateBorderSize(Window);
			Window->RealW = Window->BorderL + Window->W + Window->BorderR;
			Window->RealH = Window->BorderT + Window->H + Window->BorderB;
			bDecoratorRedraw = 1;
		}
		else
		{
			Window->BorderL = 0;
			Window->BorderR = 0;
			Window->BorderT = 0;
			Window->BorderB = 0;
			Window->RealW = Window->W;
			Window->RealH = Window->H;
		}

		Window->Renderer->Redraw(Window);
		Window->Flags |= WINFLAG_CLEAN|WINFLAG_NEEDREBLIT;
	}
	
	// Process children
	if( !(Window->Flags & WINFLAG_CHILDCLEAN) )
	{
		tWindow	*child;
		for( child = Window->FirstChild; child; child = child->NextSibling )
		{
			WM_int_UpdateWindow(child);
		}
		Window->Flags |= WINFLAG_CHILDCLEAN;
	}
	
	if( bDecoratorRedraw )
		Decorator_Redraw(Window);
}

void WM_int_BlitWindow(tWindow *Window, int bForceReblit)
{
	tWindow	*child;

	// Ignore hidden windows
	if( !(Window->Flags & WINFLAG_SHOW) )
		return ;
	
	// Duplicated position update to handle window moving
	if( (Window->Flags & WINFLAG_RELATIVE) && Window->Parent )
	{
		Window->RealX = Window->Parent->RealX + Window->Parent->BorderL + Window->X;
		Window->RealY = Window->Parent->RealY + Window->Parent->BorderT + Window->Y;
	}
	else
	{
		Window->RealX = Window->X;
		Window->RealY = Window->Y;
	}

//	_SysDebug("Blit %p (%p) to (%i,%i) %ix%i", Window, Window->RenderBuffer,
//		Window->RealX, Window->RealY, Window->RealW, Window->RealH);
	// TODO Don't blit unless:
	// a) A parent has been reblitted (thus clobbering the existing content)
	// b) A child has moved (exposing a previously hidden area)
	if( bForceReblit || (Window->Flags & WINFLAG_NEEDREBLIT) )
	{
		Video_Blit(Window->RenderBuffer, Window->RealX, Window->RealY, Window->RealW, Window->RealH);
		Window->Flags &= ~WINFLAG_NEEDREBLIT;
		bForceReblit = 1;
	}
	
	if( Window == gpWM_FocusedWindow && Window->CursorW )
	{
		Video_FillRect(
			Window->RealX + Window->BorderL + Window->CursorX,
			Window->RealY + Window->BorderT + Window->CursorY,
			Window->CursorW, Window->CursorH,
			0x000000
			);
	}

	for( child = Window->FirstChild; child; child = child->NextSibling )
	{
		WM_int_BlitWindow(child, bForceReblit);
	}
}

void WM_Update(void)
{
	// Don't redraw if nothing has changed
	if( (gpWM_RootWindow->Flags & WINFLAG_CHILDCLEAN) )
		return ;	

	// - Iterate through visible windows, updating them as needed
	WM_int_UpdateWindow(gpWM_RootWindow);
	
	// - Draw windows from back to front to the render buffer
	WM_int_BlitWindow(gpWM_RootWindow, 0);

	Video_Update();
}

