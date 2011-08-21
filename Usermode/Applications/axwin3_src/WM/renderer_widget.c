/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * render_widget.c
 * - AxWin2 Widget port
 */
#include <common.h>
#include <wm_renderer.h>

// === TYPES ===
typedef struct sWidgetWin	tWidgetWin;

// === STRUCTURES ===
struct sWidgetWin
{
	tElement	RootElement;
};

// === PROTOTYPES ===
tWindow	*Renderer_Widget_Create(int Width, int Height, int Flags);
void	Renderer_Widget_Redraw(tWindow *Window);
int	Renderer_Widget_HandleMessage(tWindow *Target, int Msg, int Len, void *Data);

// === GLOBALS ===
tWMRenderer	gRenderer_Widget = {
	.Name = "Classful",
	.CreateWindow = Renderer_Widget_Create,
	.Redraw = Renderer_Widget_Redraw,
	.HandleMessage = Renderer_Widget_HandleMessage
};

// === CODE ===
int Renderer_Widget_Init(void)
{
	WM_RegisterRenderer(&gRenderer_Widget);	

	return 0;
}

tWindow	*Renderer_Widget_Create(int Width, int Height, int Flags)
{
	// TODO: Add info
	return WM_CreateWindowStruct(tWidgetWin_Info);
}

void Renderer_Widget_Redraw(tWindow *Window)
{
}

int Renderer_Widget_HandleMessage(tWindow *Target, int Msg, int Len, void *Data)
{
	tClassfulInfo	*info = Target->RendererInfo;
	switch(Msg)
	{

	}
}

