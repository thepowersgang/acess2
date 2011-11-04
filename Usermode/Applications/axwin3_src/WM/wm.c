/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * wm.c
 * - Window manager core
 */
#include <common.h>
#include <wm_renderer.h>

// === CODE ===
void WM_RegisterRenderer(tWMRenderer *Renderer)
{
	UNIMPLEMENTED();
}

tWindow *WM_CreateWindow(tWindow *Parent, int Flags, const char *RendererName)
{
	UNIMPLEMENTED();
	
	// - Get Renderer

	// - Call create window function
	
	// - Fill common fields on that
	
	// - Return!
	return NULL;
}

tWindow *WM_CreateWindowStruct(size_t ExtraSize)
{
	return NULL;
}

void WM_Render_FilledRect(tWindow *Window, tColour Colour, int X, int Y, int W, int H)
{
	UNIMPLEMENTED();
}

