/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * wm.c
 * - Window manager core
 */
#include <common.h>
#include <wm_internals.h>

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
	// TODO: Mouse motion events
	// TODO: Send mouseup to match mousedown if the cursor moves out of a window?
}

void WM_Input_MouseButton(int X, int Y, int ButtonIndex, int Pressed)
{
//	tWindow	*win = WM_int_GetWindowAtPos(X, Y);
	
	// Send Press/Release message
}

