/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * renderer_passthru.c
 * - Passthrough window render (framebuffer essentially)
 */
#include <common.h>
#include <wm_renderer.h>

// === PROTOTYPES ===
tWindow	*Renderer_Passthru_Create(int Flags);
void	Renderer_Passthru_Redraw(tWindow *Window);
 int	Renderer_Passthru_HandleMessage(tWindow *Target, int Msg, int Len, void *Data);

// === GLOBALS ===
tWMRenderer	gRenderer_Passthru = {
	.Name = "Passthru",
	.CreateWindow = Renderer_Passthru_Create,
	.Redraw = Renderer_Passthru_Redraw,
	.HandleMessage = Renderer_Passthru_HandleMessage
};

// === CODE ===
int Renderer_Passthru_Init(void)
{
	return 0;
}

tWindow	*Renderer_Passthru_Create(int Flags)
{
	return NULL;
}

void Renderer_Passthru_Redraw(tWindow *Window)
{
	
}

int Renderer_Passthru_HandleMessage(tWindow *Target, int Msg, int Len, void *Data)
{
	return 1;
}


