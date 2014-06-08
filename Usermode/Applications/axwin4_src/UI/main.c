/*
 * AxWin4 GUI - UI Core
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Program core
 */
#include <axwin4/axwin.h>
#include <assert.h>

// === CODE ===
int main(int argc, const char *argv[])
{
	assert( AxWin4_Connect("ipcpipe:///Devices/ipcpipe/axwin4") );
	
	tAxWin4_Window	*bgwin = AxWin4_CreateWindow("background");
	
	AxWin4_MoveWindow(bgwin, 0,0);
	AxWin4_ResizeWindow(bgwin, 640,480);
	//AxWin4_SetWindowFlags(bgwin, AXWIN4_WNDFLAG_NODECORATE|AXWIN4_WNDFLAG_KEEPBELOW);
	
	// Load image
	//char *image = malloc(640*480*4);
	//AxWin4_DrawBitmap(bgwin, 0, 0, 640, 480, image);

	_SysDebug("Beginning queue");
	
	while( AxWin4_WaitEventQueue(0) )
		;
	_SysDebug("Clean exit");
	
	return 0;
}
