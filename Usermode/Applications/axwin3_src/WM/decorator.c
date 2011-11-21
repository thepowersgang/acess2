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
#include <wm_messages.h>

// === IMPORTS ===
extern tWindow	*gpWM_FocusedWindow;

// === PROTOTYPES ===
void	Decorator_UpdateBorderSize(tWindow *Window);
void	Decorator_Redraw(tWindow *Window);
 int	Decorator_HandleMessage(tWindow *Window, int Message, int Length, const void *Data);

// === CONSTANTS ===
tColour	cColourActive_Titlebar   = 0x00CC44;
tColour	cColourActive_TitleText  = 0x000000;
tColour	cColourInactive_Titlebar = 0xD0D0D0;
tColour	cColourInactive_TitleText= 0x000000;
tColour	cColour_TitleTopBorder   = 0xFFFFFF;
tColour	cColour_SideBorder       = 0x008000;
tColour	cColour_BottomBorder     = 0x008000;
 int	ciTitlebarHeight	= 18;
 int	ciSideBorderWidth	= 2;
 int	ciBottomBorderWidth	= 4;

// === CODE ===
void Decorator_UpdateBorderSize(tWindow *Window)
{
	Window->BorderT = ciTitlebarHeight;
	Window->BorderB = 0;
	Window->BorderL = 0;
	Window->BorderR = 0;
	
	if( Window->Flags & WINFLAG_MAXIMIZED )
		return ;

	Window->BorderB = ciBottomBorderWidth;
	Window->BorderR = ciSideBorderWidth;
	Window->BorderL = ciSideBorderWidth;
}

void Decorator_Redraw(tWindow *Window)
{
	 int	bActive;
	 int	text_width, text_height;
	
	// TODO: This could possibly be expensive, but is there a better way?
	{
		tWindow	*win;
		for(win = gpWM_FocusedWindow; win; win = win->Owner)
		{
			if(win == Window) {
				bActive = 1;
				break;
			}
		}
	}

	// Draw title bar
	WM_Render_FillRect(Window,
		0, -ciTitlebarHeight, Window->W, ciTitlebarHeight,
		(bActive ? cColourActive_Titlebar : cColourInactive_Titlebar)
		);
	WM_Render_FillRect(Window,
		0, -ciTitlebarHeight, Window->W, 1,
		cColour_TitleTopBorder
		);
	WM_Render_FillRect(Window,
		0, -ciTitlebarHeight, 1, ciTitlebarHeight,
		cColour_SideBorder
		);
	WM_Render_FillRect(Window,
		Window->W, -ciTitlebarHeight, 1, ciTitlebarHeight,
		cColour_SideBorder
		);

	WM_Render_GetTextDims(
		NULL,	// TODO: Select font
		Window->Title ? Window->Title : "jI", -1,
		&text_width, &text_height
		);
	WM_Render_DrawText(Window,
		ciTitlebarHeight + 4, -(ciTitlebarHeight - (ciTitlebarHeight/2 - text_height/2)),
		Window->W - ciTitlebarHeight - 4, text_height,
		NULL,	// TODO: Select font
		(bActive ? cColourActive_TitleText : cColourInactive_TitleText),
		Window->Title ? Window->Title : "--", -1
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
		ciSideBorderWidth*2+Window->W, 1,
		0x000000
		);
	WM_Render_FillRect(Window,
		-ciSideBorderWidth, Window->H+1,
		ciSideBorderWidth*2+Window->W, ciBottomBorderWidth-1,
		cColour_BottomBorder
		);
}

int Decorator_HandleMessage(tWindow *Window, int Message, int Length, const void *Data)
{
	switch(Message)
	{
	case WNDMSG_MOUSEMOVE: {
		const struct sWndMsg_MouseMove	*msg = Data;
		if(msg->Y >= 0)	return 1;	// Pass
		
		// TODO: Handle
		return 0; }
	case WNDMSG_MOUSEBTN: {
		const struct sWndMsg_MouseButton	*msg = Data;
		if(msg->Y >= 0)	return 1;	// Pass
		
		// TODO: Handle
		return 0; }
	default:	// Anything unhandled is passed on
		return 1;
	}
}

