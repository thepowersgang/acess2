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

// === GLOBALS ===
tWMRenderer	*gpWM_Renderers;
// - Render order list (gpWM_FirstWindow is rendered first)
tWindow	*gpWM_RootWindow;

// === CODE ===
void WM_Initialise(void)
{
	WM_CreateWindow(NULL, 0x8888FF, "Background");
}

void WM_RegisterRenderer(tWMRenderer *Renderer)
{
	// TODO: Catch out duplicates
	Renderer->Next = gpWM_Renderers;
	gpWM_Renderers = Renderer;
}

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
	return calloc( sizeof(tWindow) + ExtraSize, 1 );
}

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

void WM_Update(void)
{
	// - Iterate through visible windows, updating them as needed
	
	// - Draw windows from back to front to the render buffer
	
	// - Blit the buffer to the screen
}

void WM_Render_FilledRect(tWindow *Window, tColour Colour, int X, int Y, int W, int H)
{
	// Clip to window dimensions
	if(X < 0) { W += X; X = 0; }
	if(Y < 0) { H += Y; Y = 0; }
	if(X >= Window->W)	return;
	if(Y >= Window->H)	return;
	if(X + W > Window->W)	W = Window->W - X;
	if(Y + H > Window->H)	H = Window->H - Y;

	// Render to buffer
	// Create if needed?

	UNIMPLEMENTED();
}

