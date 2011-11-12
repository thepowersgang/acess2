/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * wm.c
 * - Window manager core
 */
#include <common.h>
#include <wm_internals.h>
#include <wm_messages.h>

#define MAX_BUTTONS	3

// === IMPORTS ===
extern tWindow	*gpWM_RootWindow;

// === GLOBALS ===
//! Window which will recieve the next keyboard event
tWindow	*gpWM_FocusedWindow;
//! Window in which the mouse button was originally pressed
tWindow	*gpWM_DownStartWindow[MAX_BUTTONS];

// === CODE ===
tWindow *WM_int_GetWindowAtPos(int X, int Y)
{
	tWindow	*win, *next_win, *ret;
	
	next_win = gpWM_RootWindow;

	while(next_win)
	{
		ret = next_win;
		next_win = NULL;
		for(win = ret->FirstChild; win; win = win->NextSibling)
		{
			if( !(win->Flags & WINFLAG_SHOW) )	continue ;
			if( X < win->X || X >= win->X + win->W )	continue;
			if( Y < win->Y || Y >= win->Y + win->H )	continue;
			next_win = win;	// Overwrite as we want the final rendered window
		}
	}
	
	return ret;
}

void WM_Input_MouseMoved(int OldX, int OldY, int NewX, int NewY)
{
	tWindow	*win, *newWin;
	struct sWndMsg_MouseMove	msg;
	
	win = WM_int_GetWindowAtPos(OldX, OldY);
	msg.X = NewX - win->X;
	msg.Y = NewY - win->Y;
	msg.dX = NewX - OldX;
	msg.dY = NewY - OldY;
	WM_SendMessage(NULL, win, WNDMSG_MOUSEMOVE, sizeof(msg), &msg);
	
	// If the new coordinates are not in a new window
	// NOTE: Should this handle crossing over a small window?
	// - Nah, no need
	newWin = WM_int_GetWindowAtPos(NewX, NewY);
	if(win == newWin)	return;

	// TODO: Send mouseup to match mousedown if the cursor moves out of a window?

	win = newWin;
	msg.X = NewX - win->X;
	msg.Y = NewY - win->Y;
	msg.dX = NewX - OldX;
	msg.dY = NewY - OldY;
	WM_SendMessage(NULL, win, WNDMSG_MOUSEMOVE, sizeof(msg), &msg);
}

inline void WM_Input_int_SendBtnMsg(tWindow *Win, int X, int Y, int Index, int Pressed)
{
	struct sWndMsg_MouseButton	msg;	

	msg.X = X - Win->X;
	msg.Y = Y - Win->Y;
	msg.Button = Index;
	msg.bPressed = !!Pressed;
	
	WM_SendMessage(NULL, Win, WNDMSG_MOUSEBTN, sizeof(msg), &msg);
}

void WM_Input_MouseButton(int X, int Y, int ButtonIndex, int Pressed)
{
	tWindow	*win;

	win = WM_int_GetWindowAtPos(X, Y);

	// Handle press of primary button to change focus
	if( ButtonIndex == 0 && Pressed == 1 )
	{
		_SysDebug("Gave focus to %p", win);
		WM_GiveFocus(win);
		WM_RaiseWindow(win);
	}

	// Make sure that even if the mouse has moved out of the original window,
	// mouse release messages reach the window.
	if( !Pressed && ButtonIndex < MAX_BUTTONS && gpWM_DownStartWindow[ButtonIndex] != win )
	{
		WM_Input_int_SendBtnMsg(gpWM_DownStartWindow[ButtonIndex], X, Y, ButtonIndex, 0);
	}
	if( Pressed && ButtonIndex < MAX_BUTTONS )
	{
		gpWM_DownStartWindow[ButtonIndex] = win;
	}

	// Send Press/Release message
	WM_Input_int_SendBtnMsg(win, X, Y, ButtonIndex, Pressed);
}

// --- Manipulation Functions ---
void WM_GiveFocus(tWindow *Destination)
{
	struct sWndMsg_Bool	_msg;
	
	if( gpWM_FocusedWindow == Destination )
		return ;
	
	_msg.Val = 0;
	WM_SendMessage(NULL, gpWM_FocusedWindow, WNDMSG_FOCUS, sizeof(_msg), &_msg);
	_msg.Val = 1;
	WM_SendMessage(NULL, Destination, WNDMSG_FOCUS, sizeof(_msg), &_msg);
	
	gpWM_FocusedWindow = Destination;
}

