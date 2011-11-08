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

// === CODE ===
int SoMain(void *Base, int argc, const char *argv[], const char **envp)
{
	// TODO: Parse the environment for the AXWIN3_SERVER variable
	gsAxWin3_int_ServerDesc = getenv("AXWIN3_SERVER");
	return 0;
}

void AxWin3_MainLoop(void)
{
	tAxWin_IPCMessage	*msg;
	 int	bExit = 0;	

	while(!bExit)
	{
		msg = AxWin3_int_GetIPCMessage();
		if(!msg)	continue;	

		// TODO: Handle message
		_SysDebug("oh look, a message (Type=%i, Window=%i, Len=%i)",
			msg->ID, msg->Window, msg->Size);

		AxWin3_int_HandleMessage( msg );
		
		free(msg);
	}
}

