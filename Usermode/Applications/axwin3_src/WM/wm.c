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

// === GLOBALS ===
tWMRenderer	*gpWM_Renderers;
tWindow	*gpWM_RootWindow;

// === CODE ===
void WM_Initialise(void)
{
	WM_CreateWindow(NULL, 0x0088FF, "Background");
	gpWM_RootWindow->W = giScreenWidth;
	gpWM_RootWindow->H = giScreenHeight;
	gpWM_RootWindow->Flags = WINFLAG_SHOW;
}

void WM_RegisterRenderer(tWMRenderer *Renderer)
{
	// TODO: Catch out duplicates
	Renderer->Next = gpWM_Renderers;
	gpWM_Renderers = Renderer;
}

// --- Manipulation
tWindow *WM_CreateWindow(tWindow *Parent, int RendererArg, const char *RendererName)
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
	ret->Parent = Parent;
	ret->Renderer = renderer;

	if(!Parent)
		Parent = gpWM_RootWindow;

	// Append to parent
	if(Parent)
	{
		if(Parent->LastChild)
			Parent->LastChild->NextSibling = ret;
		else
			Parent->FirstChild = ret;
		ret->PrevSibling = Parent->LastChild;
		Parent->LastChild = ret;
	}
	else
	{
		gpWM_RootWindow = ret;
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

void WM_ShowWindow(tWindow *Window, int bShow)
{
	// TODO: Message window
	if(bShow)
		Window->Flags |= WINFLAG_SHOW;
	else
		Window->Flags &= ~WINFLAG_SHOW;
}

int WM_MoveWindow(tWindow *Window, int X, int Y)
{
	// Clip coordinates
	if(X + Window->W < 0)	X = -Window->W + 1;
	if(Y + Window->H < 0)	Y = -Window->H + 1;
	if(X >= giScreenWidth)	X = giScreenWidth - 1;
	if(Y >= giScreenHeight)	Y = giScreenHeight - 1;
	
	Window->X = X;	Window->Y = Y;
	
	return 0;
}

int WM_ResizeWindow(tWindow *Window, int W, int H)
{
	if(W <= 0 || H <= 0 )	return 1;
	if(Window->X + W < 0)	Window->X = -W + 1;
	if(Window->Y + H < 0)	Window->Y = -H + 1;
	
	Window->W = W;	Window->H = H;
	
	return 0;
}

// --- Rendering / Update
void WM_int_UpdateWindow(tWindow *Window)
{
	tWindow	*child;

	// Ignore hidden windows
	if( !(Window->Flags & WINFLAG_SHOW) )
		return ;
	// Ignore unchanged windows
	if( Window->Flags & WINFLAG_CLEAN )
		return;

	// Render
	Window->Renderer->Redraw(Window);
	
	// Process children
	for( child = Window->FirstChild; child; child = child->NextSibling )
	{
		WM_int_UpdateWindow(child);
	}
	
	Window->Flags |= WINFLAG_CLEAN;
}

void WM_int_BlitWindow(tWindow *Window)
{
	tWindow	*child;

	// Ignore hidden windows
	if( !(Window->Flags & WINFLAG_SHOW) )
		return ;
	
	Video_Blit(Window->RenderBuffer, Window->X, Window->Y, Window->W, Window->H);
	
	for( child = Window->FirstChild; child; child = child->NextSibling )
	{
		WM_int_BlitWindow(child);
	}
}

void WM_Update(void)
{
	// Don't redraw if nothing has changed
	if( gpWM_RootWindow->Flags & WINFLAG_CLEAN )
		return ;	

	// - Iterate through visible windows, updating them as needed
	WM_int_UpdateWindow(gpWM_RootWindow);
	
	// - Draw windows from back to front to the render buffer
	WM_int_BlitWindow(gpWM_RootWindow);

	Video_Update();
}

// --- WM Render Routines
// TODO: Move to another file?
void WM_Render_FilledRect(tWindow *Window, tColour Colour, int X, int Y, int W, int H)
{
	uint32_t	*dest;
	 int	i;
	_SysDebug("WM_Render_FilledRect(%p, 0x%x...", Window, Colour);
	_SysDebug(" (%i,%i), %ix%i)", X, Y, W, H);
	// Clip to window dimensions
	if(X < 0) { W += X; X = 0; }
	if(Y < 0) { H += Y; Y = 0; }
	if(X >= Window->W)	return;
	if(Y >= Window->H)	return;
	if(X + W > Window->W)	W = Window->W - X;
	if(Y + H > Window->H)	H = Window->H - Y;
	_SysDebug(" Clipped to (%i,%i), %ix%i", X, Y, W, H);

	// Render to buffer
	// Create if needed?

	if(!Window->RenderBuffer) {
		Window->RenderBuffer = malloc(Window->W*Window->H*4);
	}

	dest = (uint32_t*)Window->RenderBuffer + Y*Window->W + X;
	while( H -- )
	{
		for( i = W; i --; )
			*dest++ = Colour;
		dest += Window->W - W;
	}
}

