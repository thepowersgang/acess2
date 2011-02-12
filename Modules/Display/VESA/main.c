/*
 * AcessOS 1
 * Video BIOS Extensions (Vesa) Driver
 */
#define DEBUG	0
#define VERSION	0x100

#include <acess.h>
#include <vfs.h>
#include <tpl_drv_video.h>
#include <fs_devfs.h>
#include <modules.h>
#include <vm8086.h>
#include "common.h"

// === CONSTANTS ===
#define	FLAG_LFB	0x1
#define VESA_DEFAULT_FRAMEBUFFER	(KERNEL_BASE|0xA0000)
#define BLINKING_CURSOR	1
#if BLINKING_CURSOR
# define VESA_CURSOR_PERIOD	1000
#endif

// === PROTOTYPES ===
 int	Vesa_Install(char **Arguments);
Uint64	Vesa_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	Vesa_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	Vesa_Ioctl(tVFS_Node *Node, int ID, void *Data);
 int	Vesa_Int_SetMode(int Mode);
 int	Vesa_Int_FindMode(tVideo_IOCtl_Mode *data);
 int	Vesa_Int_ModeInfo(tVideo_IOCtl_Mode *data);
void	Vesa_FlipCursor(void *Arg);
// --- 2D Acceleration Functions --
void	Vesa_2D_Fill(void *Ent, Uint16 X, Uint16 Y, Uint16 W, Uint16 H, Uint32 Colour);
void	Vesa_2D_Blit(void *Ent, Uint16 DstX, Uint16 DstY, Uint16 SrcX, Uint16 SrcY, Uint16 W, Uint16 H);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, Vesa, Vesa_Install, NULL, "PCI", "VM8086", NULL);
tDevFS_Driver	gVesa_DriverStruct = {
	NULL, "Vesa",
	{
	.Read = Vesa_Read,
	.Write = Vesa_Write,
	.IOCtl = Vesa_Ioctl
	}
};
tMutex	glVesa_Lock;
tVM8086	*gpVesa_BiosState;
 int	giVesaDriverId = -1;
// --- Video Modes ---
 int	giVesaCurrentMode = 0;
 int	giVesaCurrentFormat = VIDEO_BUFFMT_TEXT;
tVesa_Mode	*gVesa_Modes;
tVesa_Mode	*gpVesaCurMode;
 int	giVesaModeCount = 0;
// --- Framebuffer ---
char	*gpVesa_Framebuffer = (void*)VESA_DEFAULT_FRAMEBUFFER;
 int	giVesaPageCount = 0;	//!< Framebuffer size in pages
// --- Cursor Control ---
 int	giVesaCursorX = -1;
 int	giVesaCursorY = -1;
 int	giVesaCursorTimer = -1;	// Invalid timer
// --- 2D Video Stream Handlers ---
tDrvUtil_Video_2DHandlers	gVesa_2DFunctions = {
	NULL,
	Vesa_2D_Fill,
	Vesa_2D_Blit
};

// === CODE ===
int Vesa_Install(char **Arguments)
{
	tVesa_CallInfo	*info;
	tFarPtr	infoPtr;
	tVesa_CallModeInfo	*modeinfo;
	tFarPtr	modeinfoPtr;
	Uint16	*modes;
	int	i;
	
	// Allocate Info Block
	gpVesa_BiosState = VM8086_Init();
	info = VM8086_Allocate(gpVesa_BiosState, 512, &infoPtr.seg, &infoPtr.ofs);
	modeinfo = VM8086_Allocate(gpVesa_BiosState, 512, &modeinfoPtr.seg, &modeinfoPtr.ofs);
	// Set Requested Version
	memcpy(info->signature, "VBE2", 4);
	// Set Registers
	gpVesa_BiosState->AX = 0x4F00;
	gpVesa_BiosState->ES = infoPtr.seg;	gpVesa_BiosState->DI = infoPtr.ofs;
	// Call Interrupt
	VM8086_Int(gpVesa_BiosState, 0x10);
	if(gpVesa_BiosState->AX != 0x004F) {
		Log_Warning("VESA", "Vesa_Install - VESA/VBE Unsupported (AX = 0x%x)", gpVesa_BiosState->AX);
		return MODULE_ERR_NOTNEEDED;
	}
	
	//Log_Debug("VESA", "info->VideoModes = %04x:%04x", info->VideoModes.seg, info->VideoModes.ofs);
	modes = (Uint16 *) VM8086_GetPointer(gpVesa_BiosState, info->VideoModes.seg, info->VideoModes.ofs);
	
	// Read Modes
	for( giVesaModeCount = 0; modes[giVesaModeCount] != 0xFFFF; giVesaModeCount++ );
	gVesa_Modes = (tVesa_Mode *)malloc( giVesaModeCount * sizeof(tVesa_Mode) );
	
	Log_Debug("VESA", "%i Modes", giVesaModeCount);
	
	// Insert Text Mode
	gVesa_Modes[0].width = 80;
	gVesa_Modes[0].height = 25;
	gVesa_Modes[0].bpp = 12;
	gVesa_Modes[0].code = 0x3;
	gVesa_Modes[0].flags = 1;
	gVesa_Modes[0].fbSize = 80*25*2;
	gVesa_Modes[0].framebuffer = 0xB8000;
	
	for( i = 1; i < giVesaModeCount; i++ )
	{
		gVesa_Modes[i].code = modes[i];
		// Get Mode info
		gpVesa_BiosState->AX = 0x4F01;
		gpVesa_BiosState->CX = gVesa_Modes[i].code;
		gpVesa_BiosState->ES = modeinfoPtr.seg;
		gpVesa_BiosState->DI = modeinfoPtr.ofs;
		VM8086_Int(gpVesa_BiosState, 0x10);
		
		// Parse Info
		gVesa_Modes[i].flags = 0;
		if ( (modeinfo->attributes & 0x90) == 0x90 )
		{
			gVesa_Modes[i].flags |= FLAG_LFB;
			gVesa_Modes[i].framebuffer = modeinfo->physbase;
			gVesa_Modes[i].fbSize = modeinfo->Xres*modeinfo->Yres*modeinfo->bpp/8;
		} else {
			gVesa_Modes[i].framebuffer = 0;
			gVesa_Modes[i].fbSize = 0;
		}
		
		gVesa_Modes[i].pitch = modeinfo->pitch;
		gVesa_Modes[i].width = modeinfo->Xres;
		gVesa_Modes[i].height = modeinfo->Yres;
		gVesa_Modes[i].bpp = modeinfo->bpp;
		
		#if DEBUG
		Log_Log("VESA", "0x%x - %ix%ix%i",
			gVesa_Modes[i].code, gVesa_Modes[i].width, gVesa_Modes[i].height, gVesa_Modes[i].bpp);
		#endif
	}
	
	
	// Install Device
	giVesaDriverId = DevFS_AddDevice( &gVesa_DriverStruct );
	if(giVesaDriverId == -1)	return MODULE_ERR_MISC;
	
	return MODULE_ERR_OK;
}

/* Read from the framebuffer
 */
Uint64 Vesa_Read(tVFS_Node *Node, Uint64 off, Uint64 len, void *buffer)
{
	#if DEBUG >= 2
	Log("Vesa_Read: () - NULL\n");
	#endif
	return 0;
}

/**
 * \brief Write to the framebuffer
 */
Uint64 Vesa_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);

	if(Buffer == NULL) {
		LEAVE('i', 0);
		return 0;
	}

	// Default Text mode
	if( giVesaCurrentMode == 0 )
	{
		Uint16	*fb = (Uint16*)(KERNEL_BASE|0xB8000);
		tVT_Char	*chars = Buffer;
		 int	rem;
		
		Length /= sizeof(tVT_Char);
		Offset /= sizeof(tVT_Char);
		
		if( giVesaCurrentFormat != VIDEO_BUFFMT_TEXT ) {
			Log_Warning("VESA", "Vesa_Write - Mode 0 is not framebuffer");
			LEAVE('i', -1);
			return -1;
		}
		
		if( Offset + Length > 25*80 ) {
			Log_Warning("VESA", "Vesa_Write - Framebuffer Overflow");
			LEAVE('i', 0);
			return 0;
		}
		
		fb += 2*Offset;
		LOG("fb = %p", fb);
		for(rem = Length; rem --; fb ++, chars++)
		{
			if( chars->Ch < 0x80 )
				*fb = chars->Ch & 0x7F;
			else
				*fb = 0x00;
			
			*fb |= (chars->FGCol & 0x888) == 0x888 ? 0x8 : 0;
			*fb |= (chars->FGCol & 0x700) > 0x300 ? 0x4 : 0;
			*fb |= (chars->FGCol & 0x070) > 0x030 ? 0x2 : 0;
			*fb |= (chars->FGCol & 0x007) > 0x003 ? 0x1 : 0;
			*fb |= (chars->BGCol & 0x888) == 0x888 ? 0x80 : 0;
			*fb |= (chars->BGCol & 0x700) > 0x300 ? 0x40 : 0;
			*fb |= (chars->BGCol & 0x070) > 0x030 ? 0x20 : 0;
			*fb |= (chars->BGCol & 0x007) > 0x003 ? 0x10 : 0;
			//LOG("%08x (%03x,%03x) = %04x",
			//	chars->Ch, chars->BGCol, chars->FGCol, *fb);
		}
		Length *= sizeof(tVT_Char);
		LEAVE('X', Length);
		return Length;
	}

	if( gVesa_Modes[giVesaCurrentMode].framebuffer == 0 ) {
		Log_Warning("VESA", "Vesa_Write - Non-LFB Modes not yet supported.");
		LEAVE('i', 0);
		return 0;
	}
	
	// Text Mode
	switch( giVesaCurrentFormat )
	{
	case VIDEO_BUFFMT_TEXT:
		{
		tVT_Char	*chars = Buffer;
		 int	pitch = gVesa_Modes[giVesaCurrentMode].width;
		 int	widthInChars = gVesa_Modes[giVesaCurrentMode].width/giVT_CharWidth;
		 int	heightInChars = gVesa_Modes[giVesaCurrentMode].height/giVT_CharHeight;
		 int	x, y;
		Uint32	*dest = (void*)gpVesa_Framebuffer;
		 int	i;
		
		Length /= sizeof(tVT_Char);
		Offset /= sizeof(tVT_Char);
		
		LOG("gVesa_Modes[%i].width = %i", giVesaCurrentMode, gVesa_Modes[giVesaCurrentMode].width);
		x = Offset % widthInChars;
		y = Offset / widthInChars;
		LOG("(x,y) = (%i,%i) = [%i,%i]", x, y, x * giVT_CharWidth, y * giVT_CharHeight * pitch);
		LOG("(w,h) = (%i,%i) = [%i,%i]",
			(int)(Length % widthInChars),
			(int)(Length / widthInChars),
			(int)((Length % widthInChars) * giVT_CharWidth),
			(int)((Length / widthInChars) * giVT_CharHeight * pitch)
			);
		
		// Sanity Check
		if( Offset > (Uint64)(heightInChars*widthInChars) ) {
			LEAVE('i', 0);
			return 0;
		}
		if(y >= heightInChars) {
			LEAVE('i', 0);
			return 0;
		}
		
		
		if( (int)Offset + (int)Length > heightInChars*widthInChars ) {
			Log_Debug("VESA", "%i + %i > %i*%i (%i)",
				(int)Offset, (int)Length, heightInChars, widthInChars, heightInChars*widthInChars);
			Length = heightInChars*widthInChars - Offset;
			Log_Notice("VESA", "Clipping write size to %i characters", (int)Length);
		}
		
		dest += y * giVT_CharHeight * pitch;
		dest += x * giVT_CharWidth;
		
		LOG("dest = %p", dest);
		
		for( i = 0; i < (int)Length; i++ )
		{
			VT_Font_Render(
				chars->Ch,
				dest + x*giVT_CharWidth, pitch,
				VT_Colour12to24(chars->BGCol),
				VT_Colour12to24(chars->FGCol)
				);
			
			chars ++;
			x ++;
			if( x >= widthInChars ) {
				x = 0;
				y ++;
				dest += pitch*giVT_CharHeight;
			}
		}
		Length *= sizeof(tVT_Char);
		}
		break;
	
	case VIDEO_BUFFMT_FRAMEBUFFER:
		{
		Uint8	*destBuf = (Uint8*) ((Uint)gpVesa_Framebuffer + (Uint)Offset);
		
		if(gVesa_Modes[giVesaCurrentMode].fbSize < Offset+Length)
		{
			Log_Warning("VESA", "Vesa_Write - Framebuffer Overflow");
			LEAVE('i', 0);
			return 0;
		}
		
		LOG("buffer = %p", Buffer);
		LOG("Updating Framebuffer (%p to %p)", destBuf, destBuf + (Uint)Length);
		
		
		// Copy to Frambuffer
		memcpy(destBuf, Buffer, Length);
		
		LOG("BGA Framebuffer updated");
		}
		break;
	
	case VIDEO_BUFFMT_2DSTREAM:
		Length = DrvUtil_Video_2DStream(
			NULL,	// Single framebuffer, so Ent is unused
			Buffer, Length, &gVesa_2DFunctions, sizeof(gVesa_2DFunctions)
			);
		break;
	
	default:
		LEAVE('i', -1);
		return -1;
	}
	
	LEAVE('X', Length);
	return Length;
}

/**
 * \fn int Vesa_Ioctl(vfs_node *node, int id, void *data)
 * \brief Handle messages to the device
 */
int Vesa_Ioctl(tVFS_Node *Node, int ID, void *Data)
{
	 int	ret;
	//Log_Debug("VESA", "Vesa_Ioctl: (Node=%p, ID=%i, Data=%p)", Node, ID, Data);
	switch(ID)
	{
	case DRV_IOCTL_TYPE:	return DRV_TYPE_VIDEO;
	case DRV_IOCTL_IDENT:	memcpy("VESA", Data, 4);	return 1;
	case DRV_IOCTL_VERSION:	return VERSION;
	
	case VIDEO_IOCTL_GETSETMODE:
		if( !Data )	return giVesaCurrentMode;
		return Vesa_Int_SetMode( *(int*)Data );
	
	case VIDEO_IOCTL_FINDMODE:
		return Vesa_Int_FindMode((tVideo_IOCtl_Mode*)Data);
	case VIDEO_IOCTL_MODEINFO:
		return Vesa_Int_ModeInfo((tVideo_IOCtl_Mode*)Data);
	
	case VIDEO_IOCTL_SETBUFFORMAT:
		ret = giVesaCurrentFormat;
		if(Data) {
			//Log_Log("VESA", "Buffer mode to %i", *(int*)Data);
			giVesaCurrentFormat = *(int*)Data;
		}
		return ret;
	
	case VIDEO_IOCTL_SETCURSOR:	// Set cursor position
		#if !BLINKING_CURSOR
		if(giVesaCursorX > 0)
			Vesa_FlipCursor(Node);
		#endif
		giVesaCursorX = ((tVideo_IOCtl_Pos*)Data)->x;
		giVesaCursorY = ((tVideo_IOCtl_Pos*)Data)->y;
		//Log_Debug("VESA", "Cursor position (%i,%i)", giVesaCursorX, giVesaCursorY);
		if(
			giVesaCursorX < 0 || giVesaCursorY < 0
		||	giVesaCursorX >= gpVesaCurMode->width/giVT_CharWidth
		||	giVesaCursorY >= gpVesaCurMode->height/giVT_CharHeight)
		{
			#if BLINKING_CURSOR
			if(giVesaCursorTimer != -1) {
				Time_RemoveTimer(giVesaCursorTimer);
				giVesaCursorTimer = -1;
			}
			#endif
			giVesaCursorX = -1;
			giVesaCursorY = -1;
		}
		else {
			#if BLINKING_CURSOR
		//	Log_Debug("VESA", "Updating timer %i?", giVesaCursorTimer);
			if(giVesaCursorTimer == -1)
				giVesaCursorTimer = Time_CreateTimer(VESA_CURSOR_PERIOD, Vesa_FlipCursor, Node);
			#else
			Vesa_FlipCursor(Node);
			#endif
		}
		//Log_Debug("VESA", "Cursor position (%i,%i) Timer %i", giVesaCursorX, giVesaCursorY, giVesaCursorTimer);
		return 0;
	
	case VIDEO_IOCTL_REQLFB:	// Request Linear Framebuffer
		return 0;
	}
	return 0;
}

/**
 * \brief Updates the video mode
 */
int Vesa_Int_SetMode(int mode)
{
	Log_Log("VESA", "Setting mode to %i", mode);
	
	// Sanity Check values
	if(mode < 0 || mode > giVesaModeCount)	return -1;
	
	// Check for fast return
	if(mode == giVesaCurrentMode)	return 1;
	
	Time_RemoveTimer(giVesaCursorTimer);
	giVesaCursorTimer = -1;
	
	Mutex_Acquire( &glVesa_Lock );
	
	gpVesa_BiosState->AX = 0x4F02;
	gpVesa_BiosState->BX = gVesa_Modes[mode].code;
	if(gVesa_Modes[mode].flags & FLAG_LFB) {
		Log_Log("VESA", "Using LFB");
		gpVesa_BiosState->BX |= 0x4000;	// Bit 14 - Use LFB
	}
	
	// Set Mode
	VM8086_Int(gpVesa_BiosState, 0x10);
	
	// Map Framebuffer
	if( (tVAddr)gpVesa_Framebuffer != VESA_DEFAULT_FRAMEBUFFER )
		MM_UnmapHWPages((tVAddr)gpVesa_Framebuffer, giVesaPageCount);
	giVesaPageCount = (gVesa_Modes[mode].fbSize + 0xFFF) >> 12;
	gpVesa_Framebuffer = (void*)MM_MapHWPages(gVesa_Modes[mode].framebuffer, giVesaPageCount);
	
	Log_Log("VESA", "Framebuffer (Phys) = 0x%x", gVesa_Modes[mode].framebuffer);
	Log_Log("VESA", "Framebuffer (Virt) = 0x%x", gpVesa_Framebuffer);
	
	// Record Mode Set
	giVesaCurrentMode = mode;
	gpVesaCurMode = &gVesa_Modes[giVesaCurrentMode];
	
	Mutex_Release( &glVesa_Lock );
	
	return 1;
}

int Vesa_Int_FindMode(tVideo_IOCtl_Mode *data)
{
	 int	i;
	 int	best = -1, bestFactor = 1000;
	 int	factor, tmp;
	
	ENTER("idata->width idata->height idata->bpp", data->width, data->height, data->bpp);
	
	for(i=0;i<giVesaModeCount;i++)
	{
		LOG("Mode %i (%ix%ix%i)", i, gVesa_Modes[i].width, gVesa_Modes[i].height, gVesa_Modes[i].bpp);
	
		if(gVesa_Modes[i].width == data->width
		&& gVesa_Modes[i].height == data->height
		&& gVesa_Modes[i].bpp == data->bpp)
		{
			LOG("Perfect!");
			best = i;
			break;
		}
		
		tmp = gVesa_Modes[i].width * gVesa_Modes[i].height;
		tmp -= data->width * data->height;
		tmp = tmp < 0 ? -tmp : tmp;
		factor = tmp * 100 / (data->width * data->height);
		
		if( (data->bpp == 32 || data->bpp == 24)
		 && (gVesa_Modes[i].bpp == 32 || gVesa_Modes[i].bpp == 24) )
		{
			
		}
		else {
			factor *= 10;
		}
		
		LOG("factor = %i", factor);
		
		if(factor < bestFactor)
		{
			bestFactor = factor;
			best = i;
		}
	}
	data->id = best;
	data->width = gVesa_Modes[best].width;
	data->height = gVesa_Modes[best].height;
	data->bpp = gVesa_Modes[best].bpp;
	LEAVE('i', best);
	return best;
}

int Vesa_Int_ModeInfo(tVideo_IOCtl_Mode *data)
{
	if(data->id < 0 || data->id > giVesaModeCount)	return -1;
	data->width = gVesa_Modes[data->id].width;
	data->height = gVesa_Modes[data->id].height;
	data->bpp = gVesa_Modes[data->id].bpp;
	return 1;
}

/**
 * \brief Updates the state of the text cursor
 * \note Just does a bitwise not on the cursor region
 */
void Vesa_FlipCursor(void *Arg)
{
	 int	pitch = gpVesaCurMode->pitch/4;
	 int	x = giVesaCursorX*giVT_CharWidth;
	 int	y = giVesaCursorY*giVT_CharHeight;
	 int	i;
	Uint32	*fb = (void*)gpVesa_Framebuffer;
	
	//Debug("Cursor flip");
	
	// Sanity check
	if(giVesaCursorX < 0 || giVesaCursorY < 0
	|| y*pitch + x + (giVT_CharHeight-1)*pitch > (int)gpVesaCurMode->fbSize/4) {
		Log_Notice("VESA", "Cursor OOB (%i,%i)", x, y);
		giVesaCursorTimer = -1;
		return;
	}
	
	// Draw cursor
	fb += (y+1)*pitch + x;
	for( i = 1; i < giVT_CharHeight-1; i++, fb += pitch )
		*fb = ~*fb;
	
	#if BLINKING_CURSOR
	giVesaCursorTimer = Time_CreateTimer(VESA_CURSOR_PERIOD, Vesa_FlipCursor, Arg);
	#endif
}

// ------------------------
// --- 2D Accelleration ---
// ------------------------
void Vesa_2D_Fill(void *Ent, Uint16 X, Uint16 Y, Uint16 W, Uint16 H, Uint32 Colour)
{
	 int	pitch = gpVesaCurMode->pitch/4;
	Uint32	*buf = (Uint32*)gpVesa_Framebuffer + Y*pitch + X;
	while( H -- ) {
		memsetd(buf, Colour, W);
		buf += pitch;
	}
}

void Vesa_2D_Blit(void *Ent, Uint16 DstX, Uint16 DstY, Uint16 SrcX, Uint16 SrcY, Uint16 W, Uint16 H)
{
	 int	scrnpitch = gVesa_Modes[giVesaCurrentMode].pitch;
	 int	dst = DstY*scrnpitch + DstX;
	 int	src = SrcY*scrnpitch + SrcX;
	 int	tmp;
	
	//Log("Vesa_2D_Blit: (Ent=%p, DstX=%i, DstY=%i, SrcX=%i, SrcY=%i, W=%i, H=%i)",
	//	Ent, DstX, DstY, SrcX, SrcY, W, H);
	
	if(SrcX + W > gVesa_Modes[giVesaCurrentMode].width)
		W = gVesa_Modes[giVesaCurrentMode].width - SrcX;
	if(DstX + W > gVesa_Modes[giVesaCurrentMode].width)
		W = gVesa_Modes[giVesaCurrentMode].width - DstX;
	if(SrcY + H > gVesa_Modes[giVesaCurrentMode].height)
		H = gVesa_Modes[giVesaCurrentMode].height - SrcY;
	if(DstY + H > gVesa_Modes[giVesaCurrentMode].height)
		H = gVesa_Modes[giVesaCurrentMode].height - DstY;
	
	//Debug("W = %i, H = %i", W, H);
	
	if( dst > src ) {
		// Reverse copy
		dst += H*scrnpitch;
		src += H*scrnpitch;
		while( H -- ) {
			dst -= scrnpitch;
			src -= scrnpitch;
			tmp = W;
			for( tmp = W; tmp --; ) {
				*(Uint32*)(gpVesa_Framebuffer + dst + tmp) = *(Uint32*)(gpVesa_Framebuffer + src + tmp);
			}
		}
	}
	else {
		// Normal copy is OK
		while( H -- ) {
			memcpy((void*)gpVesa_Framebuffer + dst, (void*)gpVesa_Framebuffer + src, W*sizeof(Uint32));
			dst += scrnpitch;
			src += scrnpitch;
		}
	}
}
