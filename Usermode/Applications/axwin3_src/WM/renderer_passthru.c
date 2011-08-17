/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * renderer_passthru.c
 * - Passthrough window render (framebuffer essentially)
 */
#include <common.h>
#include <wm_renderer.h>

// === GLOBALS ===
tWMRenderer	gRenderer_Passthru = {
	.CreateWindow = Renderer_Passthru_Create,
	.Redraw = Renderer_Passthru_Redraw,
	.SendMessage = Renderer_Passthru_SendMessage
};

// === CODE ===
int Renderer_Passthru_Init(void)
{
	return 0;
}


