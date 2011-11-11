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

// === IMPORTS ===
extern tWindow	*gpWM_RootWindow;

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

void WM_Input_MouseButton(int X, int Y, int ButtonIndex, int Pressed)
{
	tWindow	*win = WM_int_GetWindowAtPos(X, Y);
	struct sWndMsg_MouseButton	msg;	

	// Send Press/Release message
	msg.X = X - win->X;
	msg.Y = Y - win->Y;
	msg.Button = ButtonIndex;
	msg.bPressed = !!Pressed;
	
	WM_SendMessage(NULL, win, WNDMSG_MOUSEBTN, sizeof(msg), &msg);
}

