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
#include <wm.h>
#include <string.h>

// === IMPORTS ===
extern int	giTerminalFD_Input;

// === PROTOTYPES ===
void	Video_Setup(void);
void	Video_SetCursorPos(short X, short Y);
void	Video_Update(void);
void	Video_FillRect(int X, int Y, int W, int H, uint32_t Color);

// === GLOBALS ===
 int	giVideo_CursorX;
 int	giVideo_CursorY;
uint32_t	*gpScreenBuffer;
 int	giVideo_FirstDirtyLine;
 int	giVideo_LastDirtyLine;

// === CODE ===
void Video_Setup(void)
{
	 int	tmpInt;
	
	// Open terminal
	#if 0
	giTerminalFD = open(gsTerminalDevice, OPENFLAG_READ|OPENFLAG_WRITE);
	if( giTerminalFD == -1 )
	{
		fprintf(stderr, "ERROR: Unable to open '%s' (%i)\n", gsTerminalDevice, _errno);
		exit(-1);
	}
	#else
	giTerminalFD = 1;
	giTerminalFD_Input = 0;
	// Check that the console is a VT
	// - ioctl(..., 0, NULL) returns the type, which should be 2
	if( ioctl(1, 0, NULL) != 2 )
	{
		fprintf(stderr, "stdout is not an Acess VT, can't start");
		_SysDebug("stdout is not an Acess VT, can't start");
		exit(-1);
	}
	#endif
	
	// Set mode to video
	tmpInt = TERM_MODE_FB;
	ioctl( giTerminalFD, TERM_IOCTL_MODETYPE, &tmpInt );
	
	// Get dimensions
	giScreenWidth = ioctl( giTerminalFD, TERM_IOCTL_WIDTH, NULL );
	giScreenHeight = ioctl( giTerminalFD, TERM_IOCTL_HEIGHT, NULL );

	giVideo_LastDirtyLine = giScreenHeight;
	
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
	 int	ofs = giVideo_FirstDirtyLine*giScreenWidth;
	 int	size = (giVideo_LastDirtyLine-giVideo_FirstDirtyLine)*giScreenWidth;
	
	if( giVideo_LastDirtyLine == 0 )	return;	

	_SysDebug("Video_Update - Updating lines %i to %i (0x%x+0x%x px)",
		giVideo_FirstDirtyLine, giVideo_LastDirtyLine, ofs, size);
	seek(giTerminalFD, ofs*4, 1);
	_SysDebug("Video_Update - Sending");
	write(giTerminalFD, gpScreenBuffer+ofs, size*4);
	_SysDebug("Video_Update - Done");
	giVideo_FirstDirtyLine = 0;
	giVideo_LastDirtyLine = 0;
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

void Video_FillRect(int X, int Y, int W, int H, uint32_t Colour)
{
	uint32_t	*dest;
	 int	i;
	
	if(X < 0 || Y < 0)	return;
	if(W >= giScreenWidth)	return;
	if(H >= giScreenHeight)	return;
	if(X + W >= giScreenWidth)	W = giScreenWidth - W;
	if(Y + H >= giScreenHeight)	W = giScreenHeight - H;
	
	dest = gpScreenBuffer + Y * giScreenWidth + X;
	while(H --)
	{
		for( i = W; i --; dest ++ )	*dest = Colour;
		dest += giScreenWidth - W;
	}
}

/**
 * \brief Blit an entire buffer to the screen
 * \note Assumes Pitch = 4*W
 */
void Video_Blit(uint32_t *Source, short DstX, short DstY, short W, short H)
{
	uint32_t	*buf;

	if( DstX >= giScreenWidth)	return ;
	if( DstY >= giScreenHeight)	return ;
	// TODO: Handle -ve X/Y by clipping
	if( DstX < 0 || DstY < 0 )	return ;
	// TODO: Handle out of bounds by clipping too
	if( DstX + W > giScreenWidth )	return;
	if( DstY + H > giScreenHeight )
		H = giScreenHeight - DstY;

	if( W <= 0 || H <= 0 )	return;

	if( DstX < giVideo_FirstDirtyLine )
		giVideo_FirstDirtyLine = DstY;
	if( DstY + H > giVideo_LastDirtyLine )
		giVideo_LastDirtyLine = DstY + H;
	
	buf = gpScreenBuffer + DstY*giScreenWidth + DstX;
	if(W != giScreenWidth)
	{
		while( H -- )
		{
			memcpy(buf, Source, W*4);
			Source += W;
			buf += giScreenWidth;
		}
	}
	else
	{
		memcpy(buf, Source, giScreenWidth*H*4);
	}
}

tColour Video_AlphaBlend(tColour _orig, tColour _new, uint8_t _alpha)
{
	uint16_t	ao,ro,go,bo;
	uint16_t	an,rn,gn,bn;
	if( _alpha == 0 )	return _orig;
	if( _alpha == 255 )	return _new;
	
	ao = (_orig >> 24) & 0xFF;
	ro = (_orig >> 16) & 0xFF;
	go = (_orig >>  8) & 0xFF;
	bo = (_orig >>  0) & 0xFF;
	
	an = (_new >> 24) & 0xFF;
	rn = (_new >> 16) & 0xFF;
	gn = (_new >>  8) & 0xFF;
	bn = (_new >>  0) & 0xFF;

	if( _alpha == 0x80 ) {
		ao = (ao + an) / 2;
		ro = (ro + rn) / 2;
		go = (go + gn) / 2;
		bo = (bo + bn) / 2;
	}
	else {
		ao = ao*(255-_alpha) + an*_alpha;
		ro = ro*(255-_alpha) + rn*_alpha;
		go = go*(255-_alpha) + gn*_alpha;
		bo = bo*(255-_alpha) + bn*_alpha;
		ao /= 255*2;
		ro /= 255*2;
		go /= 255*2;
		bo /= 255*2;
	}

	return (ao << 24) | (ro << 16) | (go << 8) | bo;
}

