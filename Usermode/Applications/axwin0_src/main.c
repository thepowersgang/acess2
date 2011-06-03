/*
Acess OS Window Manager
*/
#include "header.h"
#include "bitmaps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	USE_MOUSE	1

//GLOBALS
 int	giConsoleFP = -1;
 int	giScreenFP = -1;
 int	giScreenMode = 1;
 int	giScreenWidth = SCREEN_WIDTH;
 int	giScreenHeight = SCREEN_HEIGHT;
 int	giScreenDepth = 32;
 int	giScreenSize = SCREEN_WIDTH*SCREEN_HEIGHT*32/8;
Uint32	*gScreenBuffer;
#if USE_MOUSE
int	giMouseFP = -1;
Uint32		gCursorBMP[sizeof(bmpCursor)*2] = {0};
int		mouseX=0, mouseY=0;
Uint8	buttonState;
#endif

//PROTOTYPES
extern void Desktop_Init();
extern void heap_init();
void UpdateScreen();
#if USE_MOUSE
void UpdateMouse();
void BuildCursor();
void DrawCursor();
#endif
void memsetd(void *to, long val, int count);

//CODE
int main(void)
{
	 int	tmp;
	
	giConsoleFP = open("/Devices/vterm/1", OPENFLAG_WRITE);
	
	printf("AcessOS GUI version 1\n");
	printf("Opening and Initialising Video and Mouse...");
	
	#if USE_MOUSE
	giMouseFP = open("/Devices/ps2mouse", OPENFLAG_READ);
	#endif
	
	tmp = giScreenWidth;	ioctl(giConsoleFP, 4, &tmp);	// Width
	tmp = giScreenHeight;	ioctl(giConsoleFP, 5, &tmp);	// Height
	tmp = 1;	ioctl(giConsoleFP, 6, &tmp);	// Buffer Mode
	
	giScreenSize = giScreenWidth*giScreenHeight*giScreenDepth/8;
	
	#if USE_MOUSE
	ioctl(giMouseFP, 2, &giScreenWidth);	//Set Max X
	ioctl(giMouseFP, 3, &giScreenHeight);	//Set Max Y
	#endif
	printf("Done.\n");
	
	// Allocate Screen Buffer
	gScreenBuffer = malloc(giScreenSize);
	if(gScreenBuffer == NULL)
		printf("Unable to allocate double buffer (gScreenBuffer)\n");
	
	UpdateScreen();
	Desktop_Init();
	#if USE_MOUSE
	BuildCursor();	//Create Cursor
	#endif
	
	for(;;) {
		memset(gScreenBuffer, 0x6666FF, giScreenSize);	// Set Background Colour
		wmUpdateWindows();
		#if USE_MOUSE
		UpdateMouse();
		DrawCursor();
		#endif
		UpdateScreen();
	}
	
	return 1;
}

/* Copy from the buffer to the screen
 */
void UpdateScreen()
{
	printf("Updating Framebuffer.\n");
	seek(giScreenFP, 0, 1);	//SEEK SET
	write(giScreenFP, giScreenSize, gScreenBuffer);
}

#if USE_MOUSE
void UpdateMouse()
{
	struct {
		int	x, y, scroll;
		Uint8	buttons;
	} data;
	//k_printf("Updating Mouse State...");
	
	seek(giMouseFP, 0, 1);
	read(giMouseFP, sizeof(data), &data);
	
	mouseX = data.x;	mouseY = data.y;
	//Button Press
	if(data.buttons & ~buttonState) {
		//wmMessageButtonDown();
	}
	//Button Release
	if(~data.buttons & buttonState) {
		//wmMessageButtonUp();
	}
	
	buttonState = data.buttons;	//Update Button State
	//k_printf("Done.\n");
}

void DrawCursor()
{
	int i,j;
	Uint32	*buf, *bi;
	bi = gCursorBMP;
	buf = gScreenBuffer + mouseY*giScreenWidth + mouseX;
	for(i=0;i<24;i++)
	{
		for(j=0;j<16;j++)
		{
			if(*bi&0xFF000000)
			{
				buf[j] = *bi&0xFFFFFF;
			}
			bi++;
		}
		buf += giScreenWidth;
	}
}

void BuildCursor()
{
	int i,j;
	Uint32	px;
	Uint32	*buf, *bi;
	bi = bmpCursor;
	buf = gCursorBMP;
	for(i=0;i<sizeof(bmpCursor)/8;i++)
	{
		for(j=0;j<8;j++)
		{
			px = (*bi & (0xF << ((7-j)*4) )) >> ((7-j)*4);
			if( px & 0x1 )
			{
				if(px & 8)	*buf = 0xFF000000;	//Black (100% Alpha)
				else		*buf = 0xFFFFFFFF;	//White (100% Alpha)
			}
			else
				*buf = 0;	// 0% Alpha
			buf++;
		}
		bi++;
		for(j=0;j<8;j++)
		{
			px = (*bi & (0xF << ((7-j)*4) )) >> ((7-j)*4);
			if( px & 0x1 )
			{
				if(px & 8)	*buf = 0xFF000000;	// Black (100% Alpha)
				else		*buf = 0xFFFFFFFF;	// White (100% Alpha)
			}
			else
			{
				*buf = 0; // 0% Alpha
			}
			buf++;
		}
		bi++;
	}
}
#endif
