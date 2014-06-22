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
	
	unsigned int w, h;
	AxWin4_GetScreenDimensions(0, &w, &h);

	AxWin4_MoveWindow(bgwin, 0,0);
	AxWin4_ResizeWindow(bgwin, w,h);
	AxWin4_SetWindowFlags(bgwin, AXWIN4_WNDFLAG_NODECORATE|AXWIN4_WNDFLAG_KEEPBELOW);
	AxWin4_ShowWindow(bgwin, true);
	
	
	uint32_t *buf = AxWin4_GetWindowBuffer(bgwin);
	_SysDebug("buf = %p", buf);
	// Load image
	uint32_t *image = malloc(w*h*4);
	for(size_t i = 0; i < w*h; i ++ )
		image[i] = i*(0x1000000/w*h);
	
	//AxWin4_DrawBitmap(bgwin, 0, 0, w, h, (void*)image);

	_SysDebug("Beginning queue");
	
	while( AxWin4_WaitEventQueue(0) )
		;
	_SysDebug("Clean exit");
	
	return 0;
}
