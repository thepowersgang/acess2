/*
 * AxWin4 GUI - UI Core
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Program core
 */
#include <axwin4/axwin.h>
#include <assert.h>

tAxWin4_Window *CreateBGWin(int w, int h);
tAxWin4_Window *CreateTaskbar(int w, int h);

// === CODE ===
int main(int argc, const char *argv[])
{
	assert( AxWin4_Connect("ipcpipe:///Devices/ipcpipe/axwin4") );
	
	unsigned int w, h;
	AxWin4_GetScreenDimensions(0, &w, &h);
	
	tAxWin4_Window *bgwin = CreateBGWin(w, h);
	tAxWin4_Window *menu = CreateTaskbar(w, h);
	
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
	//AxWin4_DamageRect(bgwin, 0, 0, w, h);
	AxWin4_ShowWindow(bgwin, true);
	
	return bgwin;
}

tAxWin4_Window *CreateTaskbar(int w, int h)
{
	 int	winheight = 30;
	tAxWin4_Window *win = AxWin4_CreateWindow("taskbar");
	AxWin4_MoveWindow(win, 0, 0);
	AxWin4_ResizeWindow(win, w, winheight);
	
	AxWin4_SetWindowFlags(win, AXWIN4_WNDFLAG_NODECORATE);
	Taskbar_Redraw(win);
	AxWin4_ShowWindow(win, true);
	
	return win;
}

void Taskbar_Redraw(tAxWin4_Window *win)
{
	 int	w = 640;
	 int	h = 30;
	AxWin4_DrawControl(win, 0, 0, w, h, 0x01);	// Standard button, suitable for a toolbar
	AxWin4_DrawControl(win, 5, 5, h-10, h-10, 0x00);	// Standard button
	
	AxWin4_DamageRect(bgwin, 0, 0, w, h);
}

