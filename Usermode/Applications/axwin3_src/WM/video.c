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
	Video_FillRect(0, 0, giScreenWidth, giScreenHeight, 0x8080FF);

	// Set cursor position and bitmap
	ioctl(giTerminalFD, TERM_IOCTL_SETCURSORBITMAP, &cCursorBitmap);
	Video_SetCursorPos( giScreenWidth/2, giScreenHeight/2 );

	Video_Update();
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

void Video_FillRect(short X, short Y, short W, short H, uint32_t Color)
{
	 int	i;
	uint32_t	*buf = gpScreenBuffer + Y*giScreenWidth + X;
	
	_SysDebug("Video_FillRect: (X=%i, Y=%i, W=%i, H=%i, Color=%08x)",
		X, Y, W, H, Color);
	
	if(W < 0 || X < 0 || X >= giScreenWidth)	return ;
	if(X + W > giScreenWidth)	W = giScreenWidth - X;
	
	if(H < 0 || Y < 0 || Y >= giScreenHeight)	return ;
	if(Y + H > giScreenHeight)	H = giScreenHeight - Y;
	
	while( H -- )
	{
		for( i = W; i --; )
			*buf++ = Color;
		buf += giScreenWidth - W;
	}
}

void Video_DrawRect(short X, short Y, short W, short H, uint32_t Color)
{	
	Video_FillRect(X, Y, W, 1, Color);
	Video_FillRect(X, Y+H-1, W, 1, Color);
	Video_FillRect(X, Y, 1, H, Color);
	Video_FillRect(X+W-1, Y, 1, H, Color);
}

/**
 * \brief Blit an entire buffer to the screen
 * \note Assumes Pitch = 4*W
 */
void Video_Blit(uint32_t *Source, short DstX, short DstY, short W, short H)
{
	 int	i;
	uint32_t	*buf;

	_SysDebug("Video_Blit: (%p (%i, %i) %ix%i)", Source, DstX, DstY, W, H);
	
	if( DstX >= giScreenWidth)	return ;
	if( DstY >= giScreenHeight)	return ;
	// TODO: Handle -ve X/Y by clipping
	if( DstX < 0 || DstY < 0 )	return ;
	// TODO: Handle out of bounds by clipping too
	if( DstX + W > giScreenWidth )	return;
	if( DstY + H > giScreenHeight )
		H = giScreenWidth - DstY;

	if( W <= 0 || H <= 0 )	return;
	
	_SysDebug(" Clipped to (%i, %i) %ix%i", DstX, DstY, W, H);
	_SysDebug(" Source[0] = 0x%x", Source[0]);
	buf = gpScreenBuffer + DstY*giScreenWidth + DstX;
	while( H -- )
	{
		for( i = W; i --; )
			*buf++ = *Source++;
		buf += giScreenWidth - W;
	}
}

/**
 * \brief Draw an image to the screen
 * \todo Maybe have support for an offset in the image
 */
void Video_DrawImage(short X, short Y, short W, short H, tImage *Image)
{
	 int	x, y;
	uint8_t	*buf = (uint8_t *)(gpScreenBuffer + Y*giScreenWidth + X);
	uint8_t	*data;
	
	// Sanity please
	if( !Image )
		return ;
	
	// Bounds Check
	if( X >= giScreenWidth )	return ;
	if( Y >= giScreenHeight )	return ;
	
	// Wrap to image size
	if( W > Image->Width )	W = Image->Width;
	if( H > Image->Height )	H = Image->Height;
	
	// Wrap to screen size
	if( X + W > giScreenWidth )	W = giScreenWidth - X;
	if( Y + H > giScreenHeight )	H = giScreenHeight - Y;
	
	// Do the render
	data = Image->Data;
	switch( Image->Format )
	{
	case IMGFMT_BGRA:
		for( y = 0; y < H; y ++ )
		{
			 int	r, g, b, a;	// New
			 int	or, og, ob;	// Original
			for( x = 0; x < W; x ++ )
			{
				b = data[x*4+0]; g = data[x*4+1]; r = data[x*4+2]; a = data[x*4+3];
				if( a == 0 )	continue;	// 100% transparent
				ob = buf[x*4+0]; og = buf[x*4+1]; or = buf[x*4+2];
				// Handle Alpha
				switch(a)
				{
				// Transparent: Handled above
				// Solid
				case 0xFF:	break;
				// Half
				case 0x80:
					r = (or + r) / 2;
					g = (og + g) / 2;
					b = (ob + b) / 2;
					break;
				// General
				default:
					r = (or * (255-a) + r * a) / 255;
					g = (og * (255-a) + g * a) / 255;
					b = (ob * (255-a) + b * a) / 255;
					break;
				}
				buf[x*4+0] = b; buf[x*4+1] = g; buf[x*4+2] = r;
			}
			data += Image->Width * 4;
			buf += giScreenWidth * 4;
		}
		break;
	
	// RGB
	case IMGFMT_RGB:
		for( y = 0; y < H; y ++ )
		{
			for( x = 0; x < W; x ++ )
			{
				buf[x*4+0] = data[x*3+2];	// Blue
				buf[x*4+1] = data[x*3+1];	// Green
				buf[x*4+2] = data[x*3+0];	// Red
			}
			data += W * 3;
			buf += giScreenWidth * 4;
		}
		break;
	default:
		_SysDebug("ERROR: Unknown image format %i\n", Image->Format);
		break;
	}
}
