/*
 * AxWin3 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Entrypoint and setup
 */
#include <axwin3/axwin.h>
#include "include/internal.h"
#include "include/ipc.h"
#include <stdlib.h>

// === GLOBALS ===
int	giAxWin3_MainLoopExit;

// === CODE ===
int SoMain(void *Base, int argc, const char *argv[], const char **envp)
{
	// TODO: Parse the environment for the AXWIN3_SERVER variable
	gsAxWin3_int_ServerDesc = getenv("AXWIN3_SERVER");
	return 0;
}

int AxWin3_MainLoop(void)
{
	tAxWin_IPCMessage	*msg;

	giAxWin3_MainLoopExit = 0;	

	while(!giAxWin3_MainLoopExit)
	{
		msg = AxWin3_int_GetIPCMessage(0, NULL);
		if(!msg)	continue;	

		_SysDebug("AxWin3_MainLoop - Message (Type=%i, Window=%i, Len=%i)",
			msg->ID, msg->Window, msg->Size);

		AxWin3_int_HandleMessage( msg );
	}
	return giAxWin3_MainLoopExit;
}

void AxWin3_StopMainLoop(int Reason)
{
	giAxWin3_MainLoopExit = Reason;
}

void AxWin3_MessageSelect(int nFD, fd_set *FDs)
{
	tAxWin_IPCMessage *msg;
	msg = AxWin3_int_GetIPCMessage(nFD, FDs);
	if( msg )
		AxWin3_int_HandleMessage( msg );
}

