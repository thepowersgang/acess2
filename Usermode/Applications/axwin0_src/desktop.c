/*
AcessOS Window Manager
Desktop Window
*/
#include "axwin.h"
#include "desktop_font.h"
#include <string.h>

#define	DEBUG	0

#define	FGC	0xFFFFFFFF
#define	BGC	0xFF000000

// === GLOBALS ===
 void*	gHwnd = NULL;
 int	gW = 640;
 int	gH = 480;
 
// === PROTOTYPES ===
void	Desktop_Init();
 int	Desktop_WndProc(void *handle, int message, int arg1, int arg2);
void	Desktop_PrintAt(BITMAP *bmp, int x, int y, char *text);

// === CODE ===
void Desktop_Init()
{
	gHwnd = WM_CreateWindow(0, 0, -1, -1, Desktop_WndProc, WNDFLAG_NOBORDER|WNDFLAG_SHOW);
	if(gHwnd == 0)
	{
		//write(giConsoleFP, 32, "Unable to create desktop window\n");
		return;
	}
	WM_SetTitle(gHwnd, "Desktop");
}

int	Desktop_WndProc(void *handle, int message, int arg1, int arg2)
{
	switch(message)
	{
	case WM_REPAINT:
		{
		BITMAP	*bmp = (BITMAP*)arg1;
		memset( bmp->data, BGC, bmp->width*bmp->height*4 );
		Desktop_PrintAt( bmp, 0, 0, "CAB@1337!" );
		}
		break;
		
	default:
		return 0;
	}
	return 1;	// Handled
}

void Desktop_PrintAt(BITMAP *bmp, int x, int y, char *text)
{
	int j,k,w;
	Uint32	*buf;
	
	if( bmp == NULL || text == NULL )
	   return;

	buf = bmp->data;
	w = bmp->width;
	buf += y*w+x;
	while(*text)
	{
		for(j=0;j<9;j++)
		{
			int	c = cFONT_ASCII[(int)*text][j];
			for(k=0;k<8;k++)
			{
				if(c&(1<<(7-k)))	buf[j*w+k] = FGC;
				else				buf[j*w+k] = BGC;
			}
			buf[j*w+8] = BGC;
		}
		buf += 9;
		text++;
	}
}
