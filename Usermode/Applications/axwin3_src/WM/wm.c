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

// === CODE ===
void WM_RegisterRenderer(tWMRenderer *Renderer)
{
	// TODO: Catch re-adding the first somehow?
	if(Renderer->Next)	return;
	Renderer->Next = gpWM_Renderers;
	gpWM_Renderers = Renderer;
}

tWindow *WM_CreateWindow(tWindow *Parent, int Flags, const char *RendererName)
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
	ret = renderer->CreateWindow(Flags);
	
	// - Fill common fields on that
	ret->Flags = Flags;
	
	// - Return!
	return ret;
}

tWindow *WM_CreateWindowStruct(size_t ExtraSize)
{
	return calloc( sizeof(tWindow) + ExtraSize, 1 );
}

void WM_Render_FilledRect(tWindow *Window, tColour Colour, int X, int Y, int W, int H)
{
	UNIMPLEMENTED();
}

