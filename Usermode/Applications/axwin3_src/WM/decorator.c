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
#define BTN_CLOSE(win)	win->W-16, -16, 14, 14
#define BTN_MIN(win)	win->W-32, -16, 14, 14
tColour	cColourActive_Titlebar   = 0x00CC44;
tColour	cColourActive_TitleText  = 0x000000;
tColour	cColourInactive_Titlebar = 0xD0D0D0;
tColour	cColourInactive_TitleText= 0x000000;
tColour	cColour_TitleTopBorder   = 0xFFFFFF;
tColour	cColour_SideBorder       = 0x008000;
tColour	cColour_BottomBorder     = 0x008000;
tColour	cColour_CloseBtn         = 0xFF1100;
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
	 int	bActive = 0;
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
	// - Body
	WM_Render_FillRect(Window,
		0, -ciTitlebarHeight, Window->W, ciTitlebarHeight,
		(bActive ? cColourActive_Titlebar : cColourInactive_Titlebar)
		);
	// - Top Border
	WM_Render_FillRect(Window,
		0, -ciTitlebarHeight, Window->W, 1,
		cColour_TitleTopBorder
		);
	// - Sides
	WM_Render_FillRect(Window,
		0, -ciTitlebarHeight, 1, ciTitlebarHeight,
		cColour_SideBorder
		);
	WM_Render_FillRect(Window,
		Window->W, -ciTitlebarHeight, 1, ciTitlebarHeight,
		cColour_SideBorder
		);

	// Get the font height
	WM_Render_GetTextDims(
		NULL,	// TODO: Select font
		Window->Title ? Window->Title : "jI", -1,
		&text_width, &text_height
		);
	// - Use that to draw the window title on the left of the window
	WM_Render_DrawText(Window,
		ciTitlebarHeight + 4, -(ciTitlebarHeight - (ciTitlebarHeight/2 - text_height/2)),
		Window->W - ciTitlebarHeight - 4, text_height,
		NULL,	// TODO: Select font
		(bActive ? cColourActive_TitleText : cColourInactive_TitleText),
		Window->Title ? Window->Title : "--", -1
		);

	// TODO: Minimise, Maximise and Close	

	// Maximized windows don't have any other borders
	if( Window->Flags & WINFLAG_MAXIMIZED )
		return ;
	
	// Left Border
	WM_Render_FillRect(Window,
		-ciSideBorderWidth, -ciTitlebarHeight,
		ciSideBorderWidth, Window->H + ciTitlebarHeight + ciBottomBorderWidth,
		cColour_SideBorder
		);
	// Right Border
	WM_Render_FillRect(Window,
		Window->W, -ciTitlebarHeight,
		ciSideBorderWidth, Window->H + ciTitlebarHeight + ciBottomBorderWidth,
		cColour_SideBorder
		);
	// Bottom Border (hard line)
	WM_Render_FillRect(Window,
		-ciSideBorderWidth, Window->H,
		ciSideBorderWidth*2+Window->W, 1,
		0x000000
		);
	// Bottom Border
	WM_Render_FillRect(Window,
		-ciSideBorderWidth, Window->H+1,
		ciSideBorderWidth*2+Window->W, ciBottomBorderWidth-1,
		cColour_BottomBorder
		);
	
	// Buttons
	WM_Render_FillRect(Window, BTN_CLOSE(Window), cColour_CloseBtn);
	// TODO: Conditional for each
}

static inline int Decorator_INT_CoordInRange(int X, int Y, int SX, int SY, int EX, int EY)
{
	_SysDebug("(%i<=%i<%i, %i<=%i<%i", SX, X, EX, SY, Y, EY);
	return (X >= SX && X < EX && Y >= SY && Y < EY);
}
static inline int Decorator_INT_CoordInRangeR(int X, int Y, int SX, int SY, int W, int H)
{
	return Decorator_INT_CoordInRange(X, Y, SX, SY, SX+W, SY+H);
}

int Decorator_HandleMessage(tWindow *Window, int Message, int Length, const void *Data)
{
	static tWindow	*btn1_down;
	static enum {
		BTN1_MOVE,
		BTN1_RLEFT,
		BTN1_RRIGHT,
		BTN1_RBOTTOM,
	} btn1_mode;
	switch(Message)
	{
	case WNDMSG_MOUSEMOVE: {
		const struct sWndMsg_MouseMove	*msg = Data;

		if( Window && btn1_down == Window )
		{
			switch(btn1_mode)
			{
			case BTN1_MOVE:	// Move
				WM_MoveWindow(Window, Window->X + msg->dX, Window->Y + msg->dY);
				break;
			case BTN1_RLEFT:	// Resize left
				if( Window->W + msg->dX > 50 )
				{
					WM_MoveWindow(Window, Window->X + msg->dX, Window->Y);
					WM_ResizeWindow(Window, Window->W - msg->dX, Window->H);
				}
				break;
			case BTN1_RRIGHT:	// Resize right
				if( Window->W + msg->dX > 50 )
				{
					WM_ResizeWindow(Window, Window->W + msg->dX, Window->H);
				}
				break;
			case BTN1_RBOTTOM:	// Resize bottom
				if( Window->H + msg->dY > 50 )
				{
					WM_ResizeWindow(Window, Window->W, Window->H + msg->dY);
				}
				break;
			}
			return 0;
		}

		// TODO: Change cursor when hovering over edges

		if(msg->Y >= 0)	return 1;	// Pass

		// TODO: Handle
		return 0; }
	case WNDMSG_MOUSEBTN: {
		const struct sWndMsg_MouseButton	*msg = Data;
		
		// TODO: Do something with other buttons
		// - Window menu for example
		if( msg->Button != 0 )
			return 1;	// pass on

		if( !msg->bPressed )
			btn1_down = 0;		

		#define HOTSPOTR(x,y,w,h) Decorator_INT_CoordInRangeR(msg->X, msg->Y, x, y, w, h)
		#define HOTSPOTA(sx,sy,ex,ey) Decorator_INT_CoordInRange(msg->X, msg->Y, sx, sy, ew, eh)
		// Left resize border
		if( msg->bPressed && HOTSPOTR(-ciSideBorderWidth, -ciTitlebarHeight,
				ciSideBorderWidth, ciTitlebarHeight+Window->H+ciBottomBorderWidth) )
		{
			btn1_down = Window;
			btn1_mode = BTN1_RLEFT;
			return 0;
		}
		// Right resize border
		if( msg->bPressed && HOTSPOTR(Window->W, -ciTitlebarHeight,
			ciSideBorderWidth, ciTitlebarHeight+Window->H+ciBottomBorderWidth) )
		{
			btn1_down = Window;
			btn1_mode = BTN1_RRIGHT;
			return 0;
		}
		// Bottom resize border
		if( msg->bPressed && HOTSPOTR(0, Window->H,
			Window->W, ciTitlebarHeight) )
		{
			btn1_down = Window;
			btn1_mode = BTN1_RBOTTOM;
			return 0;
		}
		// Titlebar buttons
		if( msg->bPressed && Decorator_INT_CoordInRangeR(msg->X, msg->Y, BTN_CLOSE(Window)) )
		{
			WM_SendMessage(NULL, Window, WNDMSG_CLOSE, 0, NULL);
			return 0;
		}
		// Titlebar - Move
		if( msg->bPressed && msg->Y < 0 )
		{
			btn1_down = Window;
			btn1_mode = BTN1_MOVE;
			return 0;
		}
		#undef HOTSPOTR
		#undef HOTSPOTA

		return 1; }
	default:	// Anything unhandled is passed on
		return 1;
	}
}

