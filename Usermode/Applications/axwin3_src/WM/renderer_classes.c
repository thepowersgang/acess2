/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * render_classes.c
 * - Simple class based window renderer
 */
#include <common.h>
#include <wm_renderer.h>

// === GLOBALS ===
tWMRenderer	gRenderer_Class = {
	.CreateWindow = Renderer_Class_Create,
	.Redraw = Renderer_Class_Redraw,
	.SendMessage = Renderer_Class_SendMessage
};

// === CODE ===
int Renderer_Class_Init(void)
{
	return 0;
}


