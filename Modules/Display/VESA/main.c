/*
 * AcessOS 1
 * Video BIOS Extensions (Vesa) Driver
 */
#define DEBUG	0
#define VERSION	0x100

#include <acess.h>
#include <vfs.h>
#include <api_drv_video.h>
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
 int	Vesa_IOCtl(tVFS_Node *Node, int ID, void *Data);
 int	Vesa_Int_SetMode(int Mode);
 int	Vesa_Int_FindMode(tVideo_IOCtl_Mode *data);
 int	Vesa_Int_ModeInfo(tVideo_IOCtl_Mode *data);
void	Vesa_FlipCursor(void *Arg);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, Vesa, Vesa_Install, NULL, "PCI", "VM8086", NULL);
tDevFS_Driver	gVesa_DriverStruct = {
	NULL, "Vesa",
	{
	.Read = Vesa_Read,
	.Write = Vesa_Write,
	.IOCtl = Vesa_IOCtl
	}
};
tMutex	glVesa_Lock;
tVM8086	*gpVesa_BiosState;
 int	giVesaDriverId = -1;
// --- Video Modes ---
 int	giVesaCurrentMode = 0;
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
 int	gbVesa_CursorVisible = 0;
// --- 2D Video Stream Handlers ---
tDrvUtil_Video_BufInfo	gVesa_BufInfo;

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
			gVesa_Modes[i].fbSize = modeinfo->Yres*modeinfo->pitch;
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
	if( gVesa_Modes[giVesaCurrentMode].framebuffer == 0 ) {
		Log_Warning("VESA", "Vesa_Write - Non-LFB Modes not yet supported.");
		return 0;
	}

	return DrvUtil_Video_WriteLFB(&gVesa_BufInfo, Offset, Length, Buffer);
}

const char *csaVESA_IOCtls[] = {DRV_IOCTLNAMES, DRV_VIDEO_IOCTLNAMES, NULL};
/**
 * \brief Handle messages to the device
 */
int Vesa_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	 int	ret;
	//Log_Debug("VESA", "Vesa_Ioctl: (Node=%p, ID=%i, Data=%p)", Node, ID, Data);
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_VIDEO, "VESA", VERSION, csaVESA_IOCtls);

	case VIDEO_IOCTL_GETSETMODE:
		if( !Data )	return giVesaCurrentMode;
		return Vesa_Int_SetMode( *(int*)Data );
	
	case VIDEO_IOCTL_FINDMODE:
		return Vesa_Int_FindMode((tVideo_IOCtl_Mode*)Data);
	case VIDEO_IOCTL_MODEINFO:
		return Vesa_Int_ModeInfo((tVideo_IOCtl_Mode*)Data);
	
	case VIDEO_IOCTL_SETBUFFORMAT:
		DrvUtil_Video_DrawCursor( &gVesa_BufInfo, -1, -1 );
		giVesaCursorX = -1;
		#if BLINKING_CURSOR
		if(giVesaCursorTimer != -1) {
			Time_RemoveTimer(giVesaCursorTimer);
			giVesaCursorTimer = -1;
		}
		#endif
		ret = gVesa_BufInfo.BufferFormat;
		if(Data)	gVesa_BufInfo.BufferFormat = *(int*)Data;
		if(gVesa_BufInfo.BufferFormat == VIDEO_BUFFMT_TEXT)
			DrvUtil_Video_SetCursor( &gVesa_BufInfo, &gDrvUtil_TextModeCursor );
		return ret;
	
	case VIDEO_IOCTL_SETCURSOR:	// Set cursor position
		DrvUtil_Video_RemoveCursor( &gVesa_BufInfo );
		#if BLINKING_CURSOR
		if(giVesaCursorTimer != -1) {
			Time_RemoveTimer(giVesaCursorTimer);
			giVesaCursorTimer = -1;
		}
		#endif
		giVesaCursorX = ((tVideo_IOCtl_Pos*)Data)->x;
		giVesaCursorY = ((tVideo_IOCtl_Pos*)Data)->y;
		gbVesa_CursorVisible = (giVesaCursorX >= 0);
		if(gVesa_BufInfo.BufferFormat == VIDEO_BUFFMT_TEXT)
		{
			DrvUtil_Video_DrawCursor(
				&gVesa_BufInfo,
				giVesaCursorX*giVT_CharWidth,
				giVesaCursorY*giVT_CharHeight
				);
			#if BLINKING_CURSOR
			giVesaCursorTimer = Time_CreateTimer(VESA_CURSOR_PERIOD, Vesa_FlipCursor, NULL);
			#endif
		}
		else
			DrvUtil_Video_DrawCursor(
				&gVesa_BufInfo,
				giVesaCursorX,
				giVesaCursorY
				);
		return 0;
	}
	return 0;
}

/**
 * \brief Updates the video mode
 */
int Vesa_Int_SetMode(int mode)
{	
	// Sanity Check values
	if(mode < 0 || mode > giVesaModeCount)	return -1;

	Log_Log("VESA", "Setting mode to %i (%ix%i %ibpp)",
		mode,
		gVesa_Modes[mode].width, gVesa_Modes[mode].height,
		gVesa_Modes[mode].bpp
		);
	
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
	
	Log_Log("VESA", "Framebuffer (Phys) = 0x%x, (Virt) = 0x%x, Size = 0x%x",
		gVesa_Modes[mode].framebuffer, gpVesa_Framebuffer, giVesaPageCount << 12);
	
	// Record Mode Set
	giVesaCurrentMode = mode;
	gpVesaCurMode = &gVesa_Modes[giVesaCurrentMode];
	
	Mutex_Release( &glVesa_Lock );

	gVesa_BufInfo.Framebuffer = gpVesa_Framebuffer;
	gVesa_BufInfo.Pitch = gVesa_Modes[mode].pitch;
	gVesa_BufInfo.Width = gVesa_Modes[mode].width;
	gVesa_BufInfo.Height = gVesa_Modes[mode].height;
	gVesa_BufInfo.Depth = gVesa_Modes[mode].bpp;	

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
	
		if(gVesa_Modes[i].width == data->width && gVesa_Modes[i].height == data->height)
		{
			//if( (data->bpp == 32 || data->bpp == 24)
			// && (gVesa_Modes[i].bpp == 32 || gVesa_Modes[i].bpp == 24) )
			if( data->bpp == gVesa_Modes[i].bpp )
			{
				LOG("Perfect!");
				best = i;
				break;
			}
		}
		
		tmp = gVesa_Modes[i].width * gVesa_Modes[i].height;
		tmp -= data->width * data->height;
		tmp = tmp < 0 ? -tmp : tmp;
		factor = tmp * 1000 / (data->width * data->height);
		
		if( data->bpp == 8 && gVesa_Modes[i].bpp != 8 )	continue;
		if( data->bpp == 16 && gVesa_Modes[i].bpp != 16 )	continue;
		
		if( (data->bpp == 32 || data->bpp == 24)
		 && (gVesa_Modes[i].bpp == 32 || gVesa_Modes[i].bpp == 24) )
		{
			if( data->bpp == gVesa_Modes[i].bpp )
				factor /= 2;
		}
		else {
			if( data->bpp != gVesa_Modes[i].bpp )
				continue ;
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
	if( gVesa_BufInfo.BufferFormat != VIDEO_BUFFMT_TEXT )
		return ;

	if( gbVesa_CursorVisible )
		DrvUtil_Video_RemoveCursor(&gVesa_BufInfo);
	else
		DrvUtil_Video_DrawCursor(&gVesa_BufInfo,
			giVesaCursorX*giVT_CharWidth,
			giVesaCursorY*giVT_CharHeight
			);
	gbVesa_CursorVisible = !gbVesa_CursorVisible;
		
	#if BLINKING_CURSOR
	giVesaCursorTimer = Time_CreateTimer(VESA_CURSOR_PERIOD, Vesa_FlipCursor, Arg);
	#endif
}

