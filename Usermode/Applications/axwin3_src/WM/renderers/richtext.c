/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * render/richtext.c
 * - Formatted Line Editor
 */
#include <common.h>
#include <wm_renderer.h>
#include <richtext_messages.h>

// === PROTOTYPES ===
 int	Renderer_RichText_Init(void);
tWindow	*Renderer_RichText_Create(int Flags);
void	Renderer_RichText_Redraw(tWindow *Window);
 int	Renderer_RichText_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data);

// === GLOBALS ===
tWMRenderer	gRenderer_RichText = {
	.Name = "RichText",
	.CreateWindow	= Renderer_RichText_Create,
	.Redraw 	= Renderer_RichText_Redraw,
	.HandleMessage	= Renderer_RichText_HandleMessage
};

// === CODE ===
int Renderer_RichText_Init(void)
{
	WM_RegisterRenderer(&gRenderer_RichText);	
	return 0;
}

tWindow *Renderer_RichText_Create(int Flags)
{
	return NULL;
}

void Renderer_RichText_Redraw(tWindow *Window)
{
	return;
}

int Renderer_RichText_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data)
{
	switch(Msg)
	{
	case MSG_RICHTEXT_SETATTR:
		break;
	}
	return 0;
}
