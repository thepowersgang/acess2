/*
 * AxWin3 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * wm.c
 * - Core window management functions
 */
#include <axwin3/axwin.h>
#include <stdlib.h>

// === CODE ===
tHWND AxWin3_CreateWindow(tHWND Parent, const char *Renderer, int Flags)
{
	 int	dataSize = sizeof(tIPCMsg_CreateWin) + strlen(Renderer) + 1;
	tAxWin_IPCMessage	*msg;
	tIPCMsg_CreateWin	*create_win;

	msg = alloca( sizeof(tAxWin_IPCMessage) + dataSize );
	create_win = msg->Data;
	
	msg->ID = IPCMSG_CREATEWIN;
	msg->Flags = 0;
	msg->Window = Parent;	// TODO: Validation
	msg->Size = dataSize;

	create_win->NewWinID = AxWin3_int_AllocateWindowID();
	create_win->Flags = Flags;
	strcpy(create_win->Renderer, Renderer);

	return NULL;
}

void AxWin3_DestroyWindow(tHWND Window)
{
}
