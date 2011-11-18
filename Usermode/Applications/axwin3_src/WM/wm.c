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

// === IMPORTS ===
extern void	IPC_SendWMMessage(tIPC_Client *Client, uint32_t Src, uint32_t Dst, int Msg, int Len, const void *Data);

// === GLOBALS ===
tWMRenderer	*gpWM_Renderers;
tWindow	*gpWM_RootWindow;
//! Window which will recieve the next keyboard event
tWindow	*gpWM_FocusedWindow;

// === CODE ===
void WM_Initialise(void)
{
	WM_CreateWindow(NULL, NULL, 0, 0x0088FF, "Background");
	gpWM_RootWindow->W = giScreenWidth;
	gpWM_RootWindow->H = giScreenHeight;
	gpWM_RootWindow->Flags = WINFLAG_SHOW|WINFLAG_NODECORATE;
}

void WM_RegisterRenderer(tWMRenderer *Renderer)
{
	// TODO: Catch out duplicates
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

	if(!Parent)
		Parent = gpWM_RootWindow;

	// - Call create window function
	ret = renderer->CreateWindow(RendererArg);
	ret->Client = Client;
	ret->ID = ID;
	ret->Parent = Parent;
	ret->Renderer = renderer;
	ret->Flags |= WINFLAG_CLEAN;	// Needed to stop invaildate early exiting

	// Append to parent
	if(Parent)
	{
		if(Parent->LastChild)
			Parent->LastChild->NextSibling = ret;
		else
			Parent->FirstChild = ret;
		ret->PrevSibling = Parent->LastChild;
		Parent->LastChild = ret;
		ret->NextSibling = NULL;
	}
	else
	{
		gpWM_RootWindow = ret;
	}

	// Don't decorate child windows by default
	if(Parent != gpWM_RootWindow)
	{
		ret->Flags |= WINFLAG_NODECORATE;
	}
	
	// - Return!
	return ret;
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
}

void WM_FocusWindow(tWindow *Destination)
{
	struct sWndMsg_Bool	_msg;
	
	if( gpWM_FocusedWindow == Destination )
		return ;
	if( Destination && !(Destination->Flags & WINFLAG_SHOW) )
		return ;
	
	_msg.Val = 0;
	WM_SendMessage(NULL, gpWM_FocusedWindow, WNDMSG_FOCUS, sizeof(_msg), &_msg);
	_msg.Val = 1;
	WM_SendMessage(NULL, Destination, WNDMSG_FOCUS, sizeof(_msg), &_msg);
	
	gpWM_FocusedWindow = Destination;
}


void WM_ShowWindow(tWindow *Window, int bShow)
{
	// Message window
	struct sWndMsg_Bool	_msg;
	
	if( !!(Window->Flags & WINFLAG_SHOW) == bShow )
		return ;

	_msg.Val = !!bShow;
	WM_SendMessage(NULL, Window, WNDMSG_SHOW, sizeof(_msg), &_msg);
	
	if(bShow)
		Window->Flags |= WINFLAG_SHOW;
	else {
		Window->Flags &= ~WINFLAG_SHOW;
		if( Window == gpWM_FocusedWindow )
			WM_FocusWindow(Window->Parent);
	}
	// Just a little memory saving for large hidden windows
	if(Window->RenderBuffer) {
		free(Window->RenderBuffer);
		Window->RenderBuffer = NULL;
	}
	
	WM_Invalidate(Window);
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
	
	WM_Invalidate(Window);
}

int WM_MoveWindow(tWindow *Window, int X, int Y)
{
	// Clip coordinates
	if(X + Window->W < 0)	X = -Window->W + 1;
	if(Y + Window->H < 0)	Y = -Window->H + 1;
	if(X >= giScreenWidth)	X = giScreenWidth - 1;
	if(Y >= giScreenHeight)	Y = giScreenHeight - 1;
	
	Window->X = X;	Window->Y = Y;

	WM_Invalidate(Window);

	return 0;
}

int WM_ResizeWindow(tWindow *Window, int W, int H)
{
	if(W <= 0 || H <= 0 )	return 1;
	if(Window->X + W < 0)	Window->X = -W + 1;
	if(Window->Y + H < 0)	Window->Y = -H + 1;

	Window->W = W;	Window->H = H;

	if(Window->RenderBuffer) {
		free(Window->RenderBuffer);
		Window->RenderBuffer = NULL;
	}
	WM_Invalidate(Window);

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
	if(Dest == NULL)	return -2;
	if(Length > 0 && Data == NULL)	return -1;

	if( Decorator_HandleMessage(Dest, Message, Length, Data) != 1 )
	{
		// TODO: Catch errors from ->HandleMessage
		return 0;
	}
	
	// ->HandleMessage returns 1 when the message was not handled
	if( Dest->Renderer->HandleMessage(Dest, Message, Length, Data) != 1 )
	{
		// TODO: Catch errors from ->HandleMessage
		return 0;
	}

	// TODO: Implement message masking

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
		
		IPC_SendWMMessage(Dest->Client, src_id, Dest->ID, Message, Length, Data);
	}	

	return 1;
}

void WM_Invalidate(tWindow *Window)
{
	_SysDebug("Invalidating %p", Window);
	// Don't invalidate twice (speedup)
//	if( !(Window->Flags & WINFLAG_CLEAN) )	return;

	// Mark for re-render
	Window->Flags &= ~WINFLAG_CLEAN;

	// Mark up the tree that a child window has changed	
	while( (Window = Window->Parent) )
		Window->Flags &= ~WINFLAG_CHILDCLEAN;
}

// --- Rendering / Update
void WM_int_UpdateWindow(tWindow *Window)
{
	tWindow	*child;

	// Ignore hidden windows
	if( !(Window->Flags & WINFLAG_SHOW) )
		return ;
	
	// Render
	if( !(Window->Flags & WINFLAG_CLEAN) )
	{
		// Calculate RealW/RealH
		if( !(Window->Flags & WINFLAG_NODECORATE) )
		{
			_SysDebug("Applying decorations to %p", Window);
			Decorator_UpdateBorderSize(Window);
			Window->RealW = Window->BorderL + Window->W + Window->BorderR;
			Window->RealH = Window->BorderT + Window->H + Window->BorderB;
			Decorator_Redraw(Window);
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
		Window->Flags |= WINFLAG_CLEAN;
	}
	
	// Process children
	if( !(Window->Flags & WINFLAG_CHILDCLEAN) )
	{
		for( child = Window->FirstChild; child; child = child->NextSibling )
		{
			WM_int_UpdateWindow(child);
		}
		Window->Flags |= WINFLAG_CHILDCLEAN;
	}
	
}

void WM_int_BlitWindow(tWindow *Window)
{
	tWindow	*child;

	// Ignore hidden windows
	if( !(Window->Flags & WINFLAG_SHOW) )
		return ;

	_SysDebug("Blit %p to (%i,%i) %ix%i", Window, Window->X, Window->Y, Window->RealW, Window->RealH);
	Video_Blit(Window->RenderBuffer, Window->X, Window->Y, Window->RealW, Window->RealH);
	
	for( child = Window->FirstChild; child; child = child->NextSibling )
	{
		WM_int_BlitWindow(child);
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
	WM_int_BlitWindow(gpWM_RootWindow);

	Video_Update();
}

