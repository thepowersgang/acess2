/*
 * Acess2 GUI (AxWin) Version 3
 * - By John Hodge (thePowersGang)
 *
 * video.c
 * - Video methods
 */
#include <common.h>
#include <acess/sys.h>
#include <acess/devices/pty.h>
#include <acess/devices/pty_cmds.h>
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
void	Video_int_SetBufFmt(int NewFmt);
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
	 int	rv;
	
	// Open terminal
	#if 0
	giTerminalFD = _SysOpen(gsTerminalDevice, OPENFLAG_READ|OPENFLAG_WRITE);
	if( giTerminalFD == -1 )
	{
		fprintf(stderr, "ERROR: Unable to open '%s' (%i)\n", gsTerminalDevice, _errno);
		exit(-1);
	}
	#else
	giTerminalFD = 1;
	giTerminalFD_Input = 0;
	// Check that the console is a PTY
	// - _SysIOCtl(..., 0, NULL) returns the type, which should be 2
	rv = _SysIOCtl(1, DRV_IOCTL_TYPE, NULL);
	if( rv != DRV_TYPE_TERMINAL )
	{
		fprintf(stderr, "stdout is not a PTY, can't start (%i exp, %i got)\n",
			DRV_TYPE_TERMINAL, rv);
		_SysDebug("stdout is not an PTY, can't start");
		exit(-1);
	}
	#endif

	// TODO: Check terminal echoback that it does support graphical modes
	// And/or have terminal flags fetchable by the client

	// Set mode to video
	Video_int_SetBufFmt(PTYBUFFMT_FB);	

	// Get dimensions
	struct ptydims dims;
	rv = _SysIOCtl( giTerminalFD, PTY_IOCTL_GETDIMS, &dims );
	if( rv ) {
		perror("Can't get terminal dimensions (WTF?)");
		exit(-1);
	}
	giScreenWidth = dims.PW;
	giScreenHeight = dims.PH;
	if( giScreenWidth < 640 || giScreenHeight < 480 ) {
		Video_int_SetBufFmt(PTYBUFFMT_TEXT);
		_SysDebug("Resoltion too small, 640x480 reqd but %ix%i avail",
			giScreenWidth, giScreenHeight);
		exit(-1);
	}
	_SysDebug("AxWin3 running at %ix%i", dims.PW, dims.PH);

	giVideo_LastDirtyLine = giScreenHeight;
	
	// Create local framebuffer (back buffer)
	gpScreenBuffer = malloc( giScreenWidth*giScreenHeight*4 );

	// Set cursor position and bitmap
	{
	Video_int_SetBufFmt(PTYBUFFMT_2DCMD);
	struct ptycmd_header	hdr = {PTY2D_CMD_SETCURSORBMP,0,0};
	size_t size = sizeof(hdr) + sizeof(cCursorBitmap) + cCursorBitmap.W*cCursorBitmap.H*4;
	hdr.len_low = size / 4;
	hdr.len_hi = size / (256*4);
	_SysWrite(giTerminalFD, &hdr, sizeof(hdr));
	_SysDebug("size = %i (%04x:%02x * 4)", size, hdr.len_hi, hdr.len_low);
	_SysWrite(giTerminalFD, &cCursorBitmap, size-sizeof(hdr));
	}
	Video_SetCursorPos( giScreenWidth/2, giScreenHeight/2 );
}

void Video_Update(void)
{
	#if 0
	 int	ofs = giVideo_FirstDirtyLine*giScreenWidth;
	 int	size = (giVideo_LastDirtyLine-giVideo_FirstDirtyLine)*giScreenWidth;
	
	if( giVideo_LastDirtyLine == 0 )	return;	

	_SysDebug("Video_Update - Updating lines %i to %i (0x%x+0x%x px)",
		giVideo_FirstDirtyLine, giVideo_LastDirtyLine, ofs, size);
	_SysSeek(giTerminalFD, ofs*4, SEEK_SET);
	_SysDebug("Video_Update - Sending FD %i %p 0x%x", giTerminalFD, gpScreenBuffer+ofs, size*4);
	_SysWrite(giTerminalFD, gpScreenBuffer+ofs, size*4);
	_SysDebug("Video_Update - Done");
	giVideo_FirstDirtyLine = giScreenHeight;
	giVideo_LastDirtyLine = 0;
	#else
	size_t	size = giScreenHeight * giScreenWidth;
	Video_int_SetBufFmt(PTYBUFFMT_FB);
	_SysWrite(giTerminalFD, gpScreenBuffer, size*4);
	#endif
}

void Video_int_SetBufFmt(int NewFmt)
{
	static int current_fmt;
	
	if( current_fmt == NewFmt )
		return ;
	
	struct ptymode mode = {.InputMode = 0, .OutputMode = NewFmt};
	int rv = _SysIOCtl( giTerminalFD, PTY_IOCTL_SETMODE, &mode );
	if( rv ) {
		perror("Can't set PTY to framebuffer mode");
		exit(-1);
	}
	
	current_fmt = NewFmt;
}

void Video_SetCursorPos(short X, short Y)
{
	struct ptycmd_setcursorpos	cmd;
	cmd.hdr.cmd = PTY2D_CMD_SETCURSORPOS;
	cmd.hdr.len_low = sizeof(cmd)/4;
	cmd.hdr.len_hi = 0;
	cmd.x = giVideo_CursorX = X;
	cmd.y = giVideo_CursorY = Y;

	Video_int_SetBufFmt(PTYBUFFMT_2DCMD);	
	_SysWrite(giTerminalFD, &cmd, sizeof(cmd));
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
	short	drawW = W;

	if( DstX >= giScreenWidth)	return ;
	if( DstY >= giScreenHeight)	return ;
	// Drawing oob to left/top, clip
	if( DstX < 0 ) {
		Source += -DstX;
		drawW -= -DstX;
		DstX = 0;
	}
	if( DstY < 0 ) {
		Source += (-DstY)*W;
		H -= -DstY;
		DstY = 0;
	}
	if( drawW < 0 )	return ;
	// Drawing oob to the right, clip
	if( DstX + drawW > giScreenWidth ) {
		//int oldw = drawW;
		drawW = giScreenWidth - DstX;
	}
	if( DstY + H > giScreenHeight )
		H = giScreenHeight - DstY;

	if( W <= 0 || H <= 0 )	return;

	giVideo_FirstDirtyLine = MIN(DstY, giVideo_FirstDirtyLine);
	giVideo_LastDirtyLine  = MAX(DstY+H, giVideo_LastDirtyLine);
	
	buf = gpScreenBuffer + DstY*giScreenWidth + DstX;
	if(drawW != giScreenWidth || W != giScreenWidth)
	{
		while( H -- )
		{
			memcpy(buf, Source, drawW*4);
			Source += W;
			buf += giScreenWidth;
		}
	}
	else
	{
		// Only valid if copying full scanlines on both ends
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

