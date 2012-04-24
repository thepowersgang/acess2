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
#include <timers.h>

// === CONSTANTS ===
#define	FLAG_LFB	0x1
#define FLAG_POPULATED	0x2
#define FLAG_VALID	0x4
#define VESA_DEFAULT_FRAMEBUFFER	(KERNEL_BASE|0xA0000)
#define BLINKING_CURSOR	1
#if BLINKING_CURSOR
# define VESA_CURSOR_PERIOD	1000
#endif

// === PROTOTYPES ===
 int	Vesa_Install(char **Arguments);
 int	VBE_int_GetModeList(void);
size_t	Vesa_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer);
 int	Vesa_IOCtl(tVFS_Node *Node, int ID, void *Data);
 int	Vesa_Int_SetMode(int Mode);
 int	Vesa_Int_FindMode(tVideo_IOCtl_Mode *data);
 int	Vesa_Int_ModeInfo(tVideo_IOCtl_Mode *data);
void	Vesa_int_HideCursor(void);
void	Vesa_int_ShowCursor(void);
void	Vesa_FlipCursor(void *Arg);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, Vesa, Vesa_Install, NULL, "PCI", "VM8086", NULL);
tVFS_NodeType	gVesa_NodeType = {
	.Write = Vesa_Write,
	.IOCtl = Vesa_IOCtl
	};
tDevFS_Driver	gVesa_DriverStruct = {
	NULL, "Vesa",
	{.Type = &gVesa_NodeType}
	};
tMutex	glVesa_Lock;
tVM8086	*gpVesa_BiosState;
 int	giVesaDriverId = -1;
// --- Video Modes ---
 int	giVesaCurrentMode = 0;
tVesa_Mode	*gVesa_Modes;
tVesa_Mode	*gpVesaCurMode;
 int	giVesaModeCount = 0;
 int	gbVesaModesChecked;
// --- Framebuffer ---
char	*gpVesa_Framebuffer = (void*)VESA_DEFAULT_FRAMEBUFFER;
 int	giVesaPageCount = 0;	//!< Framebuffer size in pages
// --- Cursor Control ---
 int	giVesaCursorX = -1;
 int	giVesaCursorY = -1;
#if BLINKING_CURSOR
tTimer	*gpVesaCursorTimer;
#endif
 int	gbVesa_CursorVisible = 0;
// --- 2D Video Stream Handlers ---
tDrvUtil_Video_BufInfo	gVesa_BufInfo;

// === CODE ===
int Vesa_Install(char **Arguments)
{
	 int	rv;
	
	gpVesa_BiosState = VM8086_Init();
	
	if( (rv = VBE_int_GetModeList()) )
		return rv;
		
	#if BLINKING_CURSOR
	// Create blink timer
	gpVesaCursorTimer = Time_AllocateTimer( Vesa_FlipCursor, NULL );
	#endif

	// Install Device
	giVesaDriverId = DevFS_AddDevice( &gVesa_DriverStruct );
	if(giVesaDriverId == -1)	return MODULE_ERR_MISC;

	return MODULE_ERR_OK;
}

int VBE_int_GetModeList(void)
{
	tVesa_CallInfo	*info;
	tFarPtr	infoPtr;
	Uint16	*modes;
	int	i;
	
	// Allocate Info Block
	info = VM8086_Allocate(gpVesa_BiosState, 512, &infoPtr.seg, &infoPtr.ofs);
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
	
	modes = (Uint16 *) VM8086_GetPointer(gpVesa_BiosState, info->VideoModes.seg, info->VideoModes.ofs);
//	VM8086_Deallocate( gpVesa_BiosState, info );
	
	// Count Modes
	for( giVesaModeCount = 0; modes[giVesaModeCount] != 0xFFFF; giVesaModeCount++ );
	giVesaModeCount ++;	// First text mode
	gVesa_Modes = (tVesa_Mode *)malloc( giVesaModeCount * sizeof(tVesa_Mode) );
	
	Log_Debug("VBE", "%i Modes", giVesaModeCount);

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
		gVesa_Modes[i].code = modes[i-1];
	}
	
	return 0;
}

void VBE_int_FillMode_Int(int Index, tVesa_CallModeInfo *vbeinfo, tFarPtr *BufPtr)
{
	tVesa_Mode	*mode = &gVesa_Modes[Index];

	// Get Mode info
	gpVesa_BiosState->AX = 0x4F01;
	gpVesa_BiosState->CX = mode->code;
	gpVesa_BiosState->ES = BufPtr->seg;
	gpVesa_BiosState->DI = BufPtr->ofs;
	VM8086_Int(gpVesa_BiosState, 0x10);

	if( gpVesa_BiosState->AX != 0x004F ) {
		Log_Error("VESA", "Getting info on mode 0x%x failed (AX=0x%x)",
			mode->code, gpVesa_BiosState->AX);
		return ;
	}

	#define S_LOG(s, fld, fmt)	LOG(" ."#fld" = "fmt, (s).fld)
	LOG("vbeinfo[0x%x] = {", mode->code);
	S_LOG(*vbeinfo, attributes, "0x%02x");
	S_LOG(*vbeinfo, winA, "0x%02x");
	S_LOG(*vbeinfo, winB, "0x%02x");
	S_LOG(*vbeinfo, granularity, "0x%04x");
	S_LOG(*vbeinfo, winsize, "0x%04x");
	S_LOG(*vbeinfo, segmentA, "0x%04x");
	S_LOG(*vbeinfo, segmentB, "0x%04x");
	LOG(" .realFctPtr = %04x:%04x", vbeinfo->realFctPtr.seg, vbeinfo->realFctPtr.ofs);
	S_LOG(*vbeinfo, pitch, "0x%04x");
	// -- Extended
	S_LOG(*vbeinfo, Xres, "%i");
	S_LOG(*vbeinfo, Yres, "%i");
	S_LOG(*vbeinfo, Wchar, "%i");
	S_LOG(*vbeinfo, Ychar, "%i");
	S_LOG(*vbeinfo, planes, "%i");
	S_LOG(*vbeinfo, bpp, "%i");
	S_LOG(*vbeinfo, banks, "%i");
	S_LOG(*vbeinfo, memory_model, "%i");
	S_LOG(*vbeinfo, bank_size, "%i");
	S_LOG(*vbeinfo, image_pages, "%i");
	// -- VBE 1.2+
	LOG(" Red   = %i bits at %i", vbeinfo->red_mask,   vbeinfo->red_position  );
	LOG(" Green = %i bits at %i", vbeinfo->green_mask, vbeinfo->green_position);
	LOG(" Blue  = %i bits at %i", vbeinfo->blue_mask,  vbeinfo->blue_position );
	#if 0
	Uint8	rsv_mask, rsv_position;
	Uint8	directcolor_attributes;
	#endif
	// --- VBE 2.0+
	S_LOG(*vbeinfo, physbase, "0x%08x");
	S_LOG(*vbeinfo, offscreen_ptr, "0x%08x");
	S_LOG(*vbeinfo, offscreen_size_kb, "0x%04x");
	// --- VBE 3.0+
	S_LOG(*vbeinfo, lfb_pitch, "0x%04x");
	S_LOG(*vbeinfo, image_count_banked, "%i");
	S_LOG(*vbeinfo, image_count_lfb, "%i");
	LOG("}");

	mode->flags = FLAG_POPULATED;
	if( !(vbeinfo->attributes & 1) ) {
		#if DEBUG
		Log_Log("VESA", "0x%x - not supported", mode->code);
		#endif
		mode->width = 0;
		mode->height = 0;
		mode->bpp = 0;
		return ;
	}

	// Parse Info
	mode->flags |= FLAG_VALID;
	if ( (vbeinfo->attributes & 0x90) == 0x90 )
	{
		mode->flags |= FLAG_LFB;
		mode->framebuffer = vbeinfo->physbase;
		mode->fbSize = vbeinfo->Yres*vbeinfo->pitch;
	} else {
		mode->framebuffer = 0;
		mode->fbSize = 0;
	}
	
	mode->pitch = vbeinfo->pitch;
	mode->width = vbeinfo->Xres;
	mode->height = vbeinfo->Yres;
	mode->bpp = vbeinfo->bpp;
	
	#if DEBUG
	Log_Log("VESA", "#%i 0x%x - %ix%ix%i (%x)",
		Index, mode->code, mode->width, mode->height, mode->bpp, mode->flags);
	#endif

} 

void Vesa_int_FillModeList(void)
{
	if( !gbVesaModesChecked )
	{
		 int	i;
		tVesa_CallModeInfo	*modeinfo;
		tFarPtr	modeinfoPtr;
		
		modeinfo = VM8086_Allocate(gpVesa_BiosState, 256, &modeinfoPtr.seg, &modeinfoPtr.ofs);
		for( i = 1; i < giVesaModeCount; i ++ )
		{
			VBE_int_FillMode_Int(i, modeinfo, &modeinfoPtr);
		}	
//		VM8086_Deallocate( gpVesa_BiosState, modeinfo );
		
		gbVesaModesChecked = 1;
	}
}

/**
 * \brief Write to the framebuffer
 */
size_t Vesa_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer)
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
		Vesa_int_HideCursor();
		ret = gVesa_BufInfo.BufferFormat;
		if(Data)	gVesa_BufInfo.BufferFormat = *(int*)Data;
		if(gVesa_BufInfo.BufferFormat == VIDEO_BUFFMT_TEXT)
			DrvUtil_Video_SetCursor( &gVesa_BufInfo, &gDrvUtil_TextModeCursor );
		Vesa_int_ShowCursor();
		return ret;
	
	case VIDEO_IOCTL_SETCURSOR:	// Set cursor position
		Vesa_int_HideCursor();
		giVesaCursorX = ((tVideo_IOCtl_Pos*)Data)->x;
		giVesaCursorY = ((tVideo_IOCtl_Pos*)Data)->y;
		Vesa_int_ShowCursor();
		return 0;
	
	case VIDEO_IOCTL_SETCURSORBITMAP:
		DrvUtil_Video_SetCursor( &gVesa_BufInfo, Data );
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

	// Check for fast return
	if(mode == giVesaCurrentMode)	return 1;
	
	Vesa_int_FillModeList();

	Time_RemoveTimer(gpVesaCursorTimer);
	
	Mutex_Acquire( &glVesa_Lock );
	
	gpVesa_BiosState->AX = 0x4F02;
	gpVesa_BiosState->BX = gVesa_Modes[mode].code;
	if(gVesa_Modes[mode].flags & FLAG_LFB) {
		gpVesa_BiosState->BX |= 1 << 14;	// Use LFB
	}
	LOG("In : AX=%04x/BX=%04x",
		gpVesa_BiosState->AX, gpVesa_BiosState->BX);
	
	// Set Mode
	VM8086_Int(gpVesa_BiosState, 0x10);

	LOG("Out: AX = %04x", gpVesa_BiosState->AX);
	
	// Map Framebuffer
	if( (tVAddr)gpVesa_Framebuffer != VESA_DEFAULT_FRAMEBUFFER )
		MM_UnmapHWPages((tVAddr)gpVesa_Framebuffer, giVesaPageCount);
	giVesaPageCount = (gVesa_Modes[mode].fbSize + 0xFFF) >> 12;
	gpVesa_Framebuffer = (void*)MM_MapHWPages(gVesa_Modes[mode].framebuffer, giVesaPageCount);
	
	Log_Log("VESA", "Setting mode to %i 0x%x (%ix%i %ibpp) %p[0x%x] maps %P",
		mode, gVesa_Modes[mode].code,
		gVesa_Modes[mode].width, gVesa_Modes[mode].height,
		gVesa_Modes[mode].bpp,
		gpVesa_Framebuffer, giVesaPageCount << 12, gVesa_Modes[mode].framebuffer
		);
	
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

	Vesa_int_FillModeList();
	
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

	Vesa_int_FillModeList();

	data->width = gVesa_Modes[data->id].width;
	data->height = gVesa_Modes[data->id].height;
	data->bpp = gVesa_Modes[data->id].bpp;
	return 1;
}

void Vesa_int_HideCursor(void)
{
	DrvUtil_Video_RemoveCursor( &gVesa_BufInfo );
	#if BLINKING_CURSOR
	if(gpVesaCursorTimer) {
		Time_RemoveTimer(gpVesaCursorTimer);
	}
	#endif
}

void Vesa_int_ShowCursor(void)
{
	gbVesa_CursorVisible = (giVesaCursorX >= 0);
	if(gVesa_BufInfo.BufferFormat == VIDEO_BUFFMT_TEXT)
	{
		DrvUtil_Video_DrawCursor(
			&gVesa_BufInfo,
			giVesaCursorX*giVT_CharWidth,
			giVesaCursorY*giVT_CharHeight
			);
		#if BLINKING_CURSOR
		Time_ScheduleTimer( gpVesaCursorTimer, VESA_CURSOR_PERIOD );
		#endif
	}
	else
		DrvUtil_Video_DrawCursor(
			&gVesa_BufInfo,
			giVesaCursorX,
			giVesaCursorY
			);
}

/**
 * \brief Swaps the text cursor on/off
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
	Time_ScheduleTimer( gpVesaCursorTimer, VESA_CURSOR_PERIOD );
	#endif
}

