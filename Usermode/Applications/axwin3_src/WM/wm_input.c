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
#include <wm_hotkeys.h>

#define MAX_BUTTONS	3

// === IMPORTS ===
extern tWindow	*gpWM_RootWindow;
extern tWindow	*gpWM_FocusedWindow;

// === GLOBALS ===
//! Window in which the mouse button was originally pressed
tWindow	*gpWM_DownStartWindow[MAX_BUTTONS];

// === CODE ===
tWindow *WM_int_GetWindowAtPos(int X, int Y)
{
	tWindow	*win, *next_win, *ret = NULL;
	
	next_win = gpWM_RootWindow;

	while(next_win)
	{
		ret = next_win;
		next_win = NULL;
		for(win = ret->FirstChild; win; win = win->NextSibling)
		{
			if( !(win->Flags & WINFLAG_SHOW) )	continue ;
			if( X < win->RealX || X >= win->RealX + win->RealW )	continue;
			if( Y < win->RealY || Y >= win->RealY + win->RealH )	continue;
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
	msg.X = NewX - win->RealX - win->BorderL;
	msg.Y = NewY - win->RealY - win->BorderT;
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
	msg.X = NewX - win->RealX - win->BorderL;
	msg.Y = NewY - win->RealY - win->BorderT;
	msg.dX = NewX - OldX;
	msg.dY = NewY - OldY;
	WM_SendMessage(NULL, win, WNDMSG_MOUSEMOVE, sizeof(msg), &msg);
}

void WM_Input_int_SendBtnMsg(tWindow *Win, int X, int Y, int Index, int Pressed)
{
	struct sWndMsg_MouseButton	msg;	

	msg.X = X - Win->RealX - Win->BorderL;
	msg.Y = Y - Win->RealY - Win->BorderT;
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
//		_SysDebug("Gave focus to %p", win);
		WM_FocusWindow(win);
		tWindow	*tmpwin = win;
		while( tmpwin )
		{
			WM_RaiseWindow(tmpwin);
			tmpwin = tmpwin->Parent;
		}
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

void WM_Input_KeyDown(uint32_t Character, uint32_t Scancode)
{
	struct sWndMsg_KeyAction	msg;

	WM_Hotkey_KeyDown(Scancode);

	msg.KeySym = Scancode;
	msg.UCS32 = Character;
	WM_SendMessage(NULL, gpWM_FocusedWindow, WNDMSG_KEYDOWN, sizeof(msg), &msg);
}

void WM_Input_KeyFire(uint32_t Character, uint32_t Scancode)
{
	struct sWndMsg_KeyAction	msg;

	// TODO: Properly translate into KeySyms and Unicode

	msg.KeySym = Scancode;
	msg.UCS32 = Character;
	WM_SendMessage(NULL, gpWM_FocusedWindow, WNDMSG_KEYFIRE, sizeof(msg), &msg);
}

void WM_Input_KeyUp(uint32_t Character, uint32_t Scancode)
{
	struct sWndMsg_KeyAction	msg;

	WM_Hotkey_KeyUp(Scancode);

	msg.KeySym = Scancode;
	msg.UCS32 = Character;
	WM_SendMessage(NULL, gpWM_FocusedWindow, WNDMSG_KEYUP, sizeof(msg), &msg);
}

