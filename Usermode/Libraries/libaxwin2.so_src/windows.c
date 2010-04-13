/*
 * AxWin Window Manager Interface Library
 * By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess Licence. See the
 * file COPYING for details.
 * 
 * window.c - Window Control
 */
#include "common.h"

// === TYPES & STRUCTURES ===
struct sAxWin_Window
{
	struct sAxWin_Window	*Next;
	uint32_t	WmHandle;
	tAxWin_MessageCallback	Callback;
};

// === PROTOTYPES ===
tAxWin_Handle	AxWin_CreateWindow(
	int16_t X, int16_t Y, int16_t W, int16_t H,
	uint32_t Flags, tAxWin_MessageCallback *Callback
	);

// === GLOBALS ===
//mutex_t	glProcessWindows;
tAxWin_Window	*gProcessWindows;

// === CODE ===
tAxWin_Handle	AxWin_CreateWindow(
	int16_t X, int16_t Y,
	int16_t W, int16_t H,
	uint32_t Flags, tAxWin_MessageCallback *Callback)
{
	tAxWin_Message	req;
	tAxWin_Message	*msg;
	tAxWin_Window	*win;
	
	req.ID = MSG_SREQ_NEWWINDOW;
	req.Size = 1 + sizeof(struct sAxWin_SReq_NewWindow)/4;
	req.SReq_NewWindow.X = X;
	req.SReq_NewWindow.Y = Y;
	req.SReq_NewWindow.W = W;
	req.SReq_NewWindow.H = H;
	req.SReq_NewWindow.Flags = Flags;
	
	AxWin_SendMessage(&msg);
	
	for(;;)
	{
		msg = AxWin_WaitForMessage();
		
		if(msg.ID == MSG_SRSP_WINDOW)
			break;
		
		AxWin_HandleMessage(msg);
		free(msg);
	}
	
	win = malloc(sizeof(tAxWin_Window));
	win->WmHandle = msg->SRsp_Window.Handle;
	win->Callback = Callback;
	
	//mutex_acquire(glProcessWindows);
	win->Next = gProcessWindows;
	gProcessWindows = win;
	//mutex_release(glProcessWindows);
	
	return 0;
}
