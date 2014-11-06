/*
 * AxWin4 GUI - UI Core
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Program core
 */
#include <axwin4/axwin.h>
#include <assert.h>
#include "include/common.h"
#include "include/taskbar.h"

// === PROTOTYPES ===
tAxWin4_Window *CreateBGWin(int w, int h);

// === GLOABLS ===
unsigned int	giScreenWidth = 640;
unsigned int	giScreenHeight = 480;

// === CODE ===
int main(int argc, const char *argv[])
{
	assert( AxWin4_Connect("ipcpipe:///Devices/ipcpipe/axwin4") );
	
	AxWin4_GetScreenDimensions(0, &giScreenWidth, &giScreenHeight);
	
	tAxWin4_Window *bgwin = CreateBGWin(giScreenWidth, giScreenHeight);
	Taskbar_Create();
	
	_SysDebug("Beginning queue");
	while( AxWin4_WaitEventQueue(0) )
		;
	_SysDebug("Clean exit");
	
	return 0;
}

tAxWin4_Window *CreateBGWin(int w, int h)
{
	tAxWin4_Window	*bgwin = AxWin4_CreateWindow("background");
	AxWin4_MoveWindow(bgwin, 0,0);
	AxWin4_ResizeWindow(bgwin, w,h);
	AxWin4_SetWindowFlags(bgwin, AXWIN4_WNDFLAG_NODECORATE|AXWIN4_WNDFLAG_KEEPBELOW);
	
	// Load background image
	uint32_t *buf = AxWin4_GetWindowBuffer(bgwin);
	if( buf )
	{
		for( size_t y = 0; y < h; y ++ )
		{
			for(size_t x = 0; x < w; x ++ )
			{
				uint8_t	r = y * 256 / h;
				uint8_t	g = 0;
				uint8_t	b = x * 256 / w;
				buf[y*w+x] = (r << 16) | (g << 8) | b;
			}
		}
	}
	else
	{
		//AxWin4_FillRect(bgwin, 0, 0, w, h, 0x0000CC);
	}
	//AxWin4_DamageRect(bgwin, 0, 0, w, h);
	AxWin4_ShowWindow(bgwin, true);
	
	return bgwin;
}

