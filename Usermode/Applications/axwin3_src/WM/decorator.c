/*
 * Acess2 Window Manager v3 (axwin3)
 * - By John Hodge (thePowersGang)
 *
 * decorator.c
 * - Window Decorator
 */
#include <common.h>
#include <wm.h>
#include <decorator.h>

// === PROTOTYPES ===
void	Decorator_UpdateBorderSize(tWindow *Window);
void	Decorator_Redraw(tWindow *Window);
 int	Decorator_HandleMessage(tWindow *Window, int Message, int Length, void *Data);

// === CONSTANTS ===
tColour	cColourActive_Titlebar   = 0xFF8800;
tColour	cColourActive_TitleText  = 0x000000;
tColour	cColourInactive_Titlebar = 0xD0D0D0;
tColour	cColourInactive_TitleText= 0x000000;
tColour	cColour_SideBorder       = 0xD0D0D0;
tColour	cColour_BottomBorder     = 0xD0D0D0;
 int	ciTitlebarHeight	= 18;
 int	ciSideBorderWidth	= 2;
 int	ciBottomBorderWidth	= 4;

// === CODE ===
void Decorator_UpdateBorderSize(tWindow *Window)
{
	Window->BorderT = 0;
	Window->BorderB = 0;
	Window->BorderL = 0;
	Window->BorderR = 0;
	
	Window->BorderT = ciTitlebarHeight;
	if( Window->Flags & WINFLAG_MAXIMIZED )
		return ;
	
	Window->BorderB = ciBottomBorderWidth;
	Window->BorderR = ciSideBorderWidth;
	Window->BorderL = ciSideBorderWidth;
}

void Decorator_Redraw(tWindow *Window)
{
	 int	bActive = 1;
	 int	text_width, text_height;
	
	// TODO: Detect if window has focus
	
	// Draw title bar
	WM_Render_FillRect(Window,
		0, -ciTitlebarHeight, Window->W, ciTitlebarHeight,
		(bActive ? cColourActive_Titlebar : cColourInactive_Titlebar)
		);

	WM_Render_GetTextDims(
		NULL,	// TODO: Select font
		Window->Title ? Window->Title : "jI",
		&text_width, &text_height
		);
	WM_Render_DrawText(Window,
		ciTitlebarHeight + 4, -(ciTitlebarHeight - (ciTitlebarHeight/2 - text_height/2)),
		Window->W - ciTitlebarHeight - 4, text_height,
		NULL,	// TODO: Select font
		(bActive ? cColourActive_TitleText : cColourInactive_TitleText),
		Window->Title ? Window->Title : "--"
		);
	
	// Maximized windows don't have any other borders
	if( Window->Flags & WINFLAG_MAXIMIZED )
		return ;
	
	// Left
	WM_Render_FillRect(Window,
		-ciSideBorderWidth, -ciTitlebarHeight,
		ciSideBorderWidth, Window->H + ciTitlebarHeight + ciBottomBorderWidth,
		cColour_SideBorder
		);
	// Right
	WM_Render_FillRect(Window,
		Window->W, -ciTitlebarHeight,
		ciSideBorderWidth, Window->H + ciTitlebarHeight + ciBottomBorderWidth,
		cColour_SideBorder
		);
	// Bottom
	WM_Render_FillRect(Window,
		-ciSideBorderWidth, Window->H,
		ciSideBorderWidth*2+Window->W, ciBottomBorderWidth,
		cColour_BottomBorder
		);
}

int Decorator_HandleMessage(tWindow *Window, int Message, int Length, void *Data)
{
	switch(Message)
	{
	default:	// Anything unhandled is passed on
		return 1;
	}
}

