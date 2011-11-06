/*
 * Acess2 GUI (AxWin) Version 3
 * - By John Hodge (thePowersGang)
 *
 * video.c
 * - Video methods
 */
#include <common.h>
#include <acess/sys.h>
#include <acess/devices/terminal.h>
#include <image.h>
#include "resources/cursor.h"
#include <stdio.h>
#include <video.h>

// === PROTOTYPES ===
void	Video_Setup(void);
void	Video_SetCursorPos(short X, short Y);
void	Video_Update(void);
void	Video_FillRect(short X, short Y, short W, short H, uint32_t Color);
void	Video_DrawRect(short X, short Y, short W, short H, uint32_t Color);

// === GLOBALS ===
 int	giVideo_CursorX;
 int	giVideo_CursorY;
uint32_t	*gpScreenBuffer;

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
	
	// Force VT to be shown
	ioctl( giTerminalFD, TERM_IOCTL_FORCESHOW, NULL );
	
	// Create local framebuffer (back buffer)
	gpScreenBuffer = malloc( giScreenWidth*giScreenHeight*4 );

	// Set cursor position and bitmap
	ioctl(giTerminalFD, TERM_IOCTL_SETCURSORBITMAP, &cCursorBitmap);
	Video_SetCursorPos( giScreenWidth/2, giScreenHeight/2 );
}

void Video_Update(void)
{
	_SysDebug("Video_Update - gpScreenBuffer[0] = 0x%x", gpScreenBuffer[0]);
	seek(giTerminalFD, 0, 1);
	write(giTerminalFD, gpScreenBuffer, giScreenWidth*giScreenHeight*4);
	_SysDebug("Video_Update - Done");
}

void Video_SetCursorPos(short X, short Y)
{
	struct {
		uint16_t	x;
		uint16_t	y;
	} pos;
	pos.x = giVideo_CursorX = X;
	pos.y = giVideo_CursorY = Y;
	ioctl(giTerminalFD, TERM_IOCTL_GETSETCURSOR, &pos);
}

/**
 * \brief Blit an entire buffer to the screen
 * \note Assumes Pitch = 4*W
 */
void Video_Blit(uint32_t *Source, short DstX, short DstY, short W, short H)
{
	 int	i;
	uint32_t	*buf;

//	_SysDebug("Video_Blit: (%p (%i, %i) %ix%i)", Source, DstX, DstY, W, H);
	
	if( DstX >= giScreenWidth)	return ;
	if( DstY >= giScreenHeight)	return ;
	// TODO: Handle -ve X/Y by clipping
	if( DstX < 0 || DstY < 0 )	return ;
	// TODO: Handle out of bounds by clipping too
	if( DstX + W > giScreenWidth )	return;
	if( DstY + H > giScreenHeight )
		H = giScreenWidth - DstY;

	if( W <= 0 || H <= 0 )	return;
	
//	_SysDebug(" Clipped to (%i, %i) %ix%i", DstX, DstY, W, H);
//	_SysDebug(" Source[0] = 0x%x", Source[0]);
	buf = gpScreenBuffer + DstY*giScreenWidth + DstX;
	while( H -- )
	{
		for( i = W; i --; )
			*buf++ = *Source++;
		buf += giScreenWidth - W;
	}
}

