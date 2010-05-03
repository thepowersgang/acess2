/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"
#include <acess/sys.h>
#include <acess/devices/terminal.h>

// === PROTOTYPES ===
void	Video_Setup(void);
void	Video_Update(void);
void	Video_FillRect(short X, short Y, short W, short H, uint32_t Color);

// === GLOBALS ===

// === CODE ===
void Video_Setup(void)
{
	 int	tmpInt;
	
	// Open terminal
	giTerminalFD = open(gsTerminalDevice, OPENFLAG_READ|OPENFLAG_WRITE);
	if( giTerminalFD == -1 )
	{
		fprintf(stderr, "ERROR: Unable to open '%s' (%i)\n", gsTerminalDevice, _errno);
		exit(-1);
	}
	
	// Set width
	tmpInt = giScreenWidth;
	tmpInt = ioctl( giTerminalFD, TERM_IOCTL_WIDTH, &tmpInt );
	if(tmpInt != giScreenWidth)
	{
		fprintf(stderr, "Warning: Selected width (%i) is invalid, clipped to %i\n",
			giScreenWidth, tmpInt);
		giScreenWidth = tmpInt;
	}
	
	// Set height
	tmpInt = giScreenHeight;
	tmpInt = ioctl( giTerminalFD, TERM_IOCTL_HEIGHT, &tmpInt );
	if(tmpInt != giScreenHeight)
	{
		fprintf(stderr, "Warning: Selected height (%i) is invalid, clipped to %i\n",
			giScreenHeight, tmpInt);
		giScreenHeight = tmpInt;
	}
	
	// Set mode to video
	tmpInt = TERM_MODE_FB;
	ioctl( giTerminalFD, TERM_IOCTL_MODETYPE, &tmpInt );
	
	// Force VT8 to be shown
	ioctl( giTerminalFD, TERM_IOCTL_FORCESHOW, NULL );
	
	// Create local framebuffer (back buffer)
	gpScreenBuffer = malloc( giScreenWidth*giScreenHeight*4 );
	memset32( gpScreenBuffer, 0x8888FF, giScreenWidth*giScreenHeight );
	Video_Update();
}

void Video_Update(void)
{
	//seek(giTerminalFD, 0, SEEK_SET);
	seek(giTerminalFD, 0, 1);
	write(giTerminalFD, giScreenWidth*giScreenHeight*4, gpScreenBuffer);
}

void Video_FillRect(short X, short Y, short W, short H, uint32_t Color)
{
	uint32_t	*buf = gpScreenBuffer + Y*giScreenWidth + X;
	
	_SysDebug("Video_FillRect: (X=%i, Y=%i, W=%i, H=%i, Color=%08x)",
		X, Y, W, H, Color);
	
	if(W < 0 || X < 0 || X >= giScreenWidth)	return ;
	if(X + W > giScreenWidth)	W = giScreenWidth - X;
	
	if(H < 0 || Y < 0 || Y >= giScreenHeight)	return ;
	if(Y + H > giScreenHeight)	H = giScreenHeight - Y;
	
	while( H -- )
	{
		memset32( buf, Color, W );
		buf += giScreenWidth;
	}
}

void Video_DrawRect(short X, short Y, short W, short H, uint32_t Color)
{	
	Video_FillRect(X, Y, W, 1, Color);
	Video_FillRect(X, Y+H-1, W, 1, Color);
	Video_FillRect(X, Y, 1, H, Color);
	Video_FillRect(X+W-1, Y, 1, H, Color);
}

void Video_DrawText(short X, short Y, short W, short H, void *Font, int Point, uint32_t Color, char *Text)
{
	// TODO!
}
