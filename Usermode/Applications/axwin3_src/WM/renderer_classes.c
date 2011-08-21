/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * render_classes.c
 * - Simple class based window renderer
 */
#include <common.h>
#include <wm_renderer.h>

// === PROTOTYPES ===
tWindow	*Renderer_Class_Create(int Width, int Height, int Flags);
void	Renderer_Class_Redraw(tWindow *Window);
int	Renderer_Class_HandleMessage(tWindow *Target, int Msg, int Len, void *Data);

// === GLOBALS ===
tWMRenderer	gRenderer_Class = {
	.Name = "Classful",
	.CreateWindow = Renderer_Class_Create,
	.Redraw = Renderer_Class_Redraw,
	.HandleMessage = Renderer_Class_HandleMessage
};

// === CODE ===
int Renderer_Class_Init(void)
{
	WM_RegisterRenderer(&gRenderer_Class);	

	return 0;
}

tWindow	*Renderer_Class_Create(int Width, int Height, int Flags)
{
	// TODO: Add info
	return WM_CreateWindowStruct(0);
}

void Renderer_Class_Redraw(tWindow *Window)
{
	Render_DrawFilledRect(Window, info->BGColour, 0, 0, Window->W, Window->H);
}

int Renderer_Class_HandleMessage(tWindow *Target, int Msg, int Len, void *Data)
{
	tClassfulInfo	*info = Target->RendererInfo;
	switch(Msg)
	{
	case MSG_CLASSFUL_SETBGCOLOUR:
		if( Len != sizeof(uint32_t) ) return -1;
		info->BGColour = *(uint32_t*)Data;
		break;

	case MSG_CLASSFUL_SETTEXT:
		
		break;
	}
}

