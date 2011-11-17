/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * render_widget.c
 * - AxWin2 Background port
 */
#include <common.h>
#include <wm_renderer.h>

// === TYPES ===

// === STRUCTURES ===
struct sBgWin
{
	uint32_t	Colour;
};

// === PROTOTYPES ===
tWindow	*Renderer_Background_Create(int Flags);
void	Renderer_Background_Redraw(tWindow *Window);
int	Renderer_Background_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data);

// === GLOBALS ===
tWMRenderer	gRenderer_Background = {
	.Name = "Background",
	.CreateWindow = Renderer_Background_Create,
	.Redraw = Renderer_Background_Redraw,
	.HandleMessage = Renderer_Background_HandleMessage
};

// === CODE ===
int Renderer_Background_Init(void)
{
	WM_RegisterRenderer(&gRenderer_Background);	

	return 0;
}

tWindow	*Renderer_Background_Create(int Arg)
{
	tWindow	*ret;
	ret = WM_CreateWindowStruct( sizeof(struct sBgWin) );
	
	((struct sBgWin*)ret->RendererInfo)->Colour = Arg;
	
	return ret;
}

void Renderer_Background_Redraw(tWindow *Window)
{
	struct sBgWin	*info = Window->RendererInfo;
	
	WM_Render_FillRect(Window, 0, 0, 0xFFFF, 0xFFFF, info->Colour);
}

int Renderer_Background_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data)
{
	switch(Msg)
	{
	// TODO: Handle resize
	
	default:
		break;
	}
	return 0;
}




