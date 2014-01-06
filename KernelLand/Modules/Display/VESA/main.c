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
#include <limits.h>

// === CONSTANTS ===
#define USE_BIOS	1
#define VESA_DEFAULT_FRAMEBUFFER	(KERNEL_BASE|0xA0000)
#define BLINKING_CURSOR	0
#if BLINKING_CURSOR
# define VESA_CURSOR_PERIOD	1000
#endif

// === PROTOTYPES ===
 int	Vesa_Install(char **Arguments);
 int	VBE_int_GetModeList(void);
size_t	Vesa_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags);
 int	Vesa_IOCtl(tVFS_Node *Node, int ID, void *Data);
 int	Vesa_Int_SetMode(int Mode);
 int	Vesa_Int_FindMode(tVideo_IOCtl_Mode *data);
 int	Vesa_Int_ModeInfo(tVideo_IOCtl_Mode *data);
void	Vesa_int_HideCursor(void);
void	Vesa_int_ShowCursor(void);
void	Vesa_FlipCursor(void *Arg);
Uint16	VBE_int_GetWord(const tVT_Char *Char);
void	VBE_int_Text_2D_Fill(void *Ent, Uint16 X, Uint16 Y, Uint16 W, Uint16 H, Uint32 Colour);
void	VBE_int_Text_2D_Blit(void *Ent, Uint16 DstX, Uint16 DstY, Uint16 SrcX, Uint16 SrcY, Uint16 W, Uint16 H);

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
tVesa_Mode	gVesa_BootMode = {0x03, 80*8, 25*16, 80*8*2, 12, FLAG_POPULATED, 80*25*2, 0xB8000};
tVesa_Mode	*gVesa_Modes;
tVesa_Mode	*gpVesaCurMode = &gVesa_BootMode;
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
tDrvUtil_Video_2DHandlers	gVBE_Text2DFunctions = {
	NULL,
	VBE_int_Text_2D_Fill,
	VBE_int_Text_2D_Blit
};
// --- Settings ---
bool	gbVesa_DisableBIOSCalls;	// Disables calls to the BIOS
// int	gbVesa_DisableFBCache;	// Disables the main-memory framebuffer cache

// === CODE ===
int Vesa_Install(char **Arguments)
{
	for( int i = 0; Arguments && Arguments[i]; i ++ )
	{
		if( strcmp(Arguments[i], "nobios") == 0 )
			gbVesa_DisableBIOSCalls = 1;
		//else if( strcmp(Arguments[i], "nocache") == 0 )
		//	gbVesa_DisableFBCache = 1;
		else {
			Log_Notice("VBE", "Unknown argument '%s'", Arguments[i]);
		}
	}

	#if USE_BIOS
	if( !gbVesa_DisableBIOSCalls )
	{
		gpVesa_BiosState = VM8086_Init();
		
		int rv = VBE_int_GetModeList();
		if(rv)	return rv;
	}
	#endif
		
	#if BLINKING_CURSOR
	// Create blink timer
	gpVesaCursorTimer = Time_AllocateTimer( Vesa_FlipCursor, NULL );
	#endif

	// Install Device
	giVesaDriverId = DevFS_AddDevice( &gVesa_DriverStruct );
	if(giVesaDriverId == -1)	return MODULE_ERR_MISC;

	return MODULE_ERR_OK;
}

#if USE_BIOS
int VBE_int_GetModeList(void)
{
	tVesa_CallInfo	*info;
	tFarPtr	infoPtr;
	Uint16	*modes;
	
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
		Log_Warning("VBE", "Vesa_Install - VESA/VBE Unsupported (AX = 0x%x)", gpVesa_BiosState->AX);
		return MODULE_ERR_NOTNEEDED;
	}
	
	modes = (Uint16 *) VM8086_GetPointer(gpVesa_BiosState, info->VideoModes.seg, info->VideoModes.ofs);
	LOG("Virtual addres of mode list from %04x:%04x is %p",
		info->VideoModes.seg, info->VideoModes.ofs, modes);
//	VM8086_Deallocate( gpVesa_BiosState, info );
	
	// Count Modes
	for( giVesaModeCount = 0; modes[giVesaModeCount] != 0xFFFF; giVesaModeCount++ )
		;
	gVesa_Modes = (tVesa_Mode *)malloc( giVesaModeCount * sizeof(tVesa_Mode) );
	
	Log_Debug("VBE", "%i Modes", giVesaModeCount);

	// Insert Text Mode
	
	for( int i = 0; i < giVesaModeCount; i++ )
	{
		gVesa_Modes[i].code = modes[i];
	}
	
	return 0;
}

int VBE_int_GetModeInfo(Uint16 Code, tFarPtr *BufPtr)
{
	// Get Mode info
	gpVesa_BiosState->AX = 0x4F01;
	gpVesa_BiosState->CX = Code;
	gpVesa_BiosState->ES = BufPtr->seg;
	gpVesa_BiosState->DI = BufPtr->ofs;
	VM8086_Int(gpVesa_BiosState, 0x10);

	if( gpVesa_BiosState->AX != 0x004F ) {
		Log_Error("VBE", "Getting info on mode 0x%x failed (AX=0x%x)",
			Code, gpVesa_BiosState->AX);
		return 1;
	}
	return 0;
}
#endif


void VBE_int_FillMode_Int(tVesa_Mode *Mode, const tVesa_CallModeInfo *vbeinfo)
{
	#if 1
	#define S_LOG(s, fld, fmt)	LOG(" ."#fld" = "fmt, (s).fld)
	LOG("vbeinfo[0x%x] = {", Mode->code);
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
	#endif
	
	Mode->flags = FLAG_POPULATED;
	// Check if this mode is supported by hardware
	if( !(vbeinfo->attributes & 1) )
	{
		#if DEBUG
		Log_Log("VBE", "0x%x - not supported", Mode->code);
		#endif
		Mode->width = 0;
		Mode->height = 0;
		Mode->bpp = 0;
		return ;
	}

	// Parse Info
	Mode->flags |= FLAG_VALID;
	switch( vbeinfo->attributes & 0x90 )	// LFB, Graphics
	{
	case 0x00:	// Banked, Text
	case 0x10:	// Banked, Graphics
	case 0x80:	// Linear, Text (?)
		Mode->width = 0;
		Mode->height = 0;
		Mode->bpp = 0;
		return ;
	case 0x90:
		Mode->flags |= FLAG_LFB|FLAG_GRAPHICS;
		Mode->framebuffer = vbeinfo->physbase;
		Mode->fbSize = vbeinfo->Yres*vbeinfo->pitch;
		break;
	}
	
	Mode->pitch = vbeinfo->pitch;
	Mode->width = vbeinfo->Xres;
	Mode->height = vbeinfo->Yres;
	Mode->bpp = vbeinfo->bpp;
	
	#if DEBUG
	Log_Log("VBE", "0x%x - %ix%ix%i (%x)",
		Mode->code, Mode->width, Mode->height, Mode->bpp, Mode->flags);
	#endif

} 

void VBE_int_SetBootMode(Uint16 ModeID, const void *ModeInfo)
{
	gVesa_BootMode.code = ModeID;
	VBE_int_FillMode_Int(&gVesa_BootMode, ModeInfo);
}

void Vesa_int_FillModeList(void)
{
	#if USE_BIOS
	if( !gbVesaModesChecked && !gbVesa_DisableBIOSCalls )
	{
		tVesa_CallModeInfo	*modeinfo;
		tFarPtr	modeinfoPtr;
		
		modeinfo = VM8086_Allocate(gpVesa_BiosState, 256, &modeinfoPtr.seg, &modeinfoPtr.ofs);
		for( int i = 0; i < giVesaModeCount; i ++ )
		{
			if( VBE_int_GetModeInfo(gVesa_Modes[i].code, &modeinfoPtr) == 0 )
			{
				VBE_int_FillMode_Int( &gVesa_Modes[i], modeinfo );
			}
		}	
//		VM8086_Deallocate( gpVesa_BiosState, modeinfo );
		
		gbVesaModesChecked = 1;
	}
	#endif
}

/**
 * \brief Write to the framebuffer
 */
size_t Vesa_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags)
{
	switch( gpVesaCurMode->flags & (FLAG_LFB|FLAG_GRAPHICS) )
	{
	case 0:
		// EGA text mode translation
		switch( gVesa_BufInfo.BufferFormat )
		{
		case VIDEO_BUFFMT_TEXT: {
			 int	num = Length / sizeof(tVT_Char);
			 int	ofs = Offset / sizeof(tVT_Char);
			 int	i = 0;
			const tVT_Char	*chars = Buffer;
			
			for( ; num--; i ++, ofs ++)
			{
				Uint16	word = VBE_int_GetWord( &chars[i] );
				((Uint16*)gVesa_BufInfo.Framebuffer)[ ofs ] = word;
			}
			
			return Length; }
		case VIDEO_BUFFMT_2DSTREAM:
			return DrvUtil_Video_2DStream(NULL, Buffer, Length,
				&gVBE_Text2DFunctions, sizeof(gVBE_Text2DFunctions));
		default:
			Log_Warning("VBE", "TODO: Alternate modes in EGA text mode");
			return 0;
		}
		return 0;
	case FLAG_LFB|FLAG_GRAPHICS:
		// Framebuffer modes - use DrvUtil Video
		return DrvUtil_Video_WriteLFB(&gVesa_BufInfo, Offset, Length, Buffer);
	default:
		Log_Warning("VBE", "TODO: _Write %s%s",
			(gpVesaCurMode->flags & FLAG_LFB ? "FLAG_LFB" : ""),
			(gpVesaCurMode->flags & FLAG_GRAPHICS ? "FLAG_GRAPHICS" : "")
			);
		return 0;
	}
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
	BASE_IOCTLS(DRV_TYPE_VIDEO, "VBE", VERSION, csaVESA_IOCtls);

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
		if( gpVesaCurMode->flags & FLAG_LFB )
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
	tVesa_Mode	*modeptr;
	// Check for fast return
	if(mode == giVesaCurrentMode)	return 1;
	
	// Special case: Boot mode
	if( mode == -1 )
		modeptr = &gVesa_BootMode;
	else if( 0 <= mode && mode < giVesaModeCount )
		modeptr = &gVesa_Modes[mode];
	else
		return -1;
	
	Vesa_int_FillModeList();

	#if BLINKING_CURSOR
	Time_RemoveTimer(gpVesaCursorTimer);
	#endif
	
	Mutex_Acquire( &glVesa_Lock );

	#if USE_BIOS
	if( gbVesa_DisableBIOSCalls )
	{
		ASSERT(mode == -1);
	}
	else
	{
		gpVesa_BiosState->AX = 0x4F02;
		gpVesa_BiosState->BX = modeptr->code;
		if(modeptr->flags & FLAG_LFB) {
			gpVesa_BiosState->BX |= 1 << 14;	// Use LFB
		}
		LOG("In : AX=%04x/BX=%04x", gpVesa_BiosState->AX, gpVesa_BiosState->BX);
		
		// Set Mode
		VM8086_Int(gpVesa_BiosState, 0x10);

		LOG("Out: AX = %04x", gpVesa_BiosState->AX);
	}
	#else
	ASSERT(mode == -1);
	#endif
	
	// Map Framebuffer
	if( gpVesaCurMode )
	{
		if( gpVesaCurMode->framebuffer < 1024*1024 )
			;
		else
			MM_UnmapHWPages((tVAddr)gpVesa_Framebuffer, giVesaPageCount);
	}
	giVesaPageCount = (modeptr->fbSize + 0xFFF) >> 12;
	if( modeptr->framebuffer < 1024*1024 )
		gpVesa_Framebuffer = (void*)(KERNEL_BASE|modeptr->framebuffer);
	else
		gpVesa_Framebuffer = (void*)MM_MapHWPages(modeptr->framebuffer, giVesaPageCount);
	
	Log_Log("VBE", "Setting mode to %i 0x%x (%ix%i %ibpp) %p[0x%x] maps %P",
		mode, modeptr->code,
		modeptr->width, modeptr->height,
		modeptr->bpp,
		gpVesa_Framebuffer, giVesaPageCount << 12, modeptr->framebuffer
		);
	
	// Record Mode Set
	giVesaCurrentMode = mode;
	gpVesaCurMode = modeptr;
	
	Mutex_Release( &glVesa_Lock );

	// TODO: Disableable backbuffer
	gVesa_BufInfo.BackBuffer  = realloc(gVesa_BufInfo.BackBuffer,
		modeptr->height * modeptr->pitch);
	gVesa_BufInfo.Framebuffer = gpVesa_Framebuffer;
	gVesa_BufInfo.Pitch = modeptr->pitch;
	gVesa_BufInfo.Width = modeptr->width;
	gVesa_BufInfo.Height = modeptr->height;
	gVesa_BufInfo.Depth = modeptr->bpp;	

	return 1;
}

int VBE_int_MatchModes(tVideo_IOCtl_Mode *ReqMode, tVesa_Mode *ThisMode)
{
	if( ThisMode->bpp == 0 ) {
		Log_Warning("VBE", "VESA mode %x (%ix%i) has 0bpp",
			ThisMode->code, ThisMode->width, ThisMode->height);
		return INT_MAX;
	}
	LOG("Matching %ix%i %ibpp", ThisMode->width, ThisMode->height, ThisMode->bpp);
	if(ThisMode->width == ReqMode->width && ThisMode->height == ReqMode->height)
	{
		//if( (data->bpp == 32 || data->bpp == 24)
		// && (gVesa_Modes[i].bpp == 32 || gVesa_Modes[i].bpp == 24) )
		if( ReqMode->bpp == ThisMode->bpp )
		{
			LOG("Perfect!");
			return -1;
		}
	}
	
	int tmp = ThisMode->width * ThisMode->height - ReqMode->width * ReqMode->height;
	tmp = tmp < 0 ? -tmp : tmp;
	unsigned int factor = (Uint64)tmp * 1000 / (ReqMode->width * ReqMode->height);
	if( ThisMode->bpp > ReqMode->bpp )
		factor += ThisMode->bpp - ReqMode->bpp;
	else
		factor += ReqMode->bpp - ThisMode->bpp;

	if( ReqMode->bpp == ThisMode->bpp )
		factor /= 2;
	else
	{
		if( ReqMode->bpp == 8 && ThisMode->bpp != 8 )	factor *= 4;
		if( ReqMode->bpp == 16 && ThisMode->bpp != 16 )	factor *= 4;
		
		if( (ReqMode->bpp == 32 || ReqMode->bpp == 24)
		 && (ThisMode->bpp == 32 || ThisMode->bpp == 24) )
		{
			// NC
		}
		else {
			if( ReqMode->bpp < ThisMode->bpp )
				factor *= ThisMode->bpp / ReqMode->bpp + 1;
			else
				factor *= ReqMode->bpp / ThisMode->bpp + 1;
		}
	}
	
	return factor;
}

int Vesa_Int_FindMode(tVideo_IOCtl_Mode *data)
{
	 int	best = -1;
	
	ENTER("idata->width idata->height idata->bpp", data->width, data->height, data->bpp);

	Vesa_int_FillModeList();

	int bestFactor = VBE_int_MatchModes(data, &gVesa_BootMode);	
	tVesa_Mode *bestPtr = &gVesa_BootMode;

	for(int i = 0; bestFactor > 0 && i < giVesaModeCount; i ++)
	{
		LOG("Mode %i (%ix%ix%i)", i, gVesa_Modes[i].width, gVesa_Modes[i].height, gVesa_Modes[i].bpp);
	
		int factor = VBE_int_MatchModes(data, &gVesa_Modes[i]);
		
		LOG("factor = %i, bestFactor = %i", factor, bestFactor);
		
		if(factor < bestFactor)
		{
			bestFactor = factor;
			best = i;
			bestPtr = &gVesa_Modes[i];
		}
	}
	data->id = best;
	data->width = bestPtr->width;
	data->height = bestPtr->height;
	data->bpp = bestPtr->bpp;
	LEAVE('i', best);
	return best;
}

int Vesa_Int_ModeInfo(tVideo_IOCtl_Mode *data)
{
	tVesa_Mode	*modeptr;
	if( data->id == -1 )
		modeptr = &gVesa_BootMode;
	else if( 0 <= data->id && data->id < giVesaModeCount)
		modeptr = &gVesa_Modes[data->id];
	else
		return 0;

	Vesa_int_FillModeList();

	data->width  = modeptr->width;
	data->height = modeptr->height;
	data->bpp    = modeptr->bpp;
	return 1;
}

void Vesa_int_HideCursor(void)
{
	DrvUtil_Video_RemoveCursor( &gVesa_BufInfo );
	if( gpVesaCurMode->flags & FLAG_LFB )
	{
		#if BLINKING_CURSOR
		if(gpVesaCursorTimer) {
			Time_RemoveTimer(gpVesaCursorTimer);
		}
		#endif
	}
}

void Vesa_int_ShowCursor(void)
{
	if( gpVesaCurMode && gpVesaCurMode->flags & FLAG_LFB )
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
	else
	{
		DrvUtil_Video_RemoveCursor( &gVesa_BufInfo );
	}
}

/**
 * \brief Swaps the text cursor on/off
 */
void Vesa_FlipCursor(void *Arg)
{
	if( gVesa_BufInfo.BufferFormat != VIDEO_BUFFMT_TEXT )
		return ;

	if( gpVesaCurMode && gpVesaCurMode->flags & FLAG_LFB )
	{
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
}

// ---
// Helpers for text mode
// ---

/**
 * \fn Uint8 VGA_int_GetColourNibble(Uint16 col)
 * \brief Converts a 12-bit colour into a VGA 4-bit colour
 */
Uint8 VBE_int_GetColourNibble(Uint16 col)
{
	Uint8	ret = 0;
	 int	bright = 0;
	
	col = col & 0xCCC;
	col = ((col>>2)&3) | ((col>>4)&0xC) | ((col>>6)&0x30);
	bright = ( (col & 2 ? 1 : 0) + (col & 8 ? 1 : 0) + (col & 32 ? 1 : 0) ) / 2;
	
	switch(col)
	{
	//	Black
	case 0x00:	ret = 0x0;	break;
	// Dark Grey
	case 0x15:	ret = 0x8;	break;
	// Blues
	case 0x01:
	case 0x02:	ret = 0x1;	break;
	case 0x03:	ret = 0x9;	break;
	// Green
	case 0x04:
	case 0x08:	ret = 0x2;	break;
	case 0x0C:	ret = 0xA;	break;
	// Reds
	case 0x10:
	case 0x20:	ret = 0x4;	break;
	case 0x30:	ret = 0xC;	break;
	// Light Grey
	case 0x2A:	ret = 0x7;	break;
	// White
	case 0x3F:	ret = 0xF;	break;
	
	default:
		ret |= (col & 0x03 ? 1 : 0);
		ret |= (col & 0x0C ? 2 : 0);
		ret |= (col & 0x30 ? 4 : 0);
		ret |= (bright ? 8 : 0);
		break;
	}
	return ret;
}

/**
 * \brief Convers a character structure to a VGA character word
 */
Uint16 VBE_int_GetWord(const tVT_Char *Char)
{
	Uint16	ret;
	Uint16	col;
	
	// Get Character
	if(Char->Ch < 128)
		ret = Char->Ch;
	else {
		switch(Char->Ch)
		{
		default:	ret = 0;	break;
		}
	}
	
	col = VBE_int_GetColourNibble(Char->BGCol);
	ret |= col << 12;
	
	col = VBE_int_GetColourNibble(Char->FGCol);
	ret |= col << 8;
	
	return ret;
}

void VBE_int_Text_2D_Fill(void *Ent, Uint16 X, Uint16 Y, Uint16 W, Uint16 H, Uint32 Colour)
{
	const int	charw = 8;
	const int	charh = 16;
	const int	tw = gpVesaCurMode->width / charw;
	const int	th = gpVesaCurMode->height / charh;

	X /= charw;
	W /= charw;
	Y /= charh;
	H /= charh;

	tVT_Char	ch;
	ch.Ch = 0x20;
	ch.BGCol  = (Colour & 0x0F0000) >> (16-8);
	ch.BGCol |= (Colour & 0x000F00) >> (8-4);
	ch.BGCol |= (Colour & 0x00000F);
	ch.FGCol = 0;
	Uint16 word = VBE_int_GetWord(&ch);

	Log("Fill (%i,%i) %ix%i with 0x%x", X, Y, W, H, word);

	if( X >= tw || Y >= th )	return ;
	if( X + W > tw )	W = tw - X;
	if( Y + H > th )	H = th - Y;

	Uint16	*buf = (Uint16*)gpVesa_Framebuffer + Y*tw + X;

	
	while( H -- ) {
		for( int i = 0; i < W; i ++ )
			*buf++ = word;
		buf += tw - W;
	}
}

void VBE_int_Text_2D_Blit(void *Ent, Uint16 DstX, Uint16 DstY, Uint16 SrcX, Uint16 SrcY, Uint16 W, Uint16 H)
{
	const int	charw = 8;
	const int	charh = 16;
	const int	tw = gpVesaCurMode->width / charw;
	const int	th = gpVesaCurMode->height / charh;

	DstX /= charw;
	SrcX /= charw;
	W    /= charw;

	DstY /= charh;
	SrcY /= charh;
	H    /= charh;

//	Log("(%i,%i) from (%i,%i) %ix%i", DstX, DstY, SrcX, SrcY, W, H);

	if( SrcX >= tw || SrcY >= th )	return ;
	if( SrcX + W > tw )	W = tw - SrcX;
	if( SrcY + H > th )	H = th - SrcY;
	if( DstX >= tw || DstY >= th )	return ;
	if( DstX + W > tw )	W = tw - DstX;
	if( DstY + H > th )	H = th - DstY;


	Uint16	*src = (Uint16*)gpVesa_Framebuffer + SrcY*tw + SrcX;
	Uint16	*dst = (Uint16*)gpVesa_Framebuffer + DstY*tw + DstX;

	if( src > dst )
	{
		// Simple copy
		while( H-- ) {
			memcpy(dst, src, W*2);
			dst += tw;
			src += tw;
		}
	}
	else
	{
		dst += H*tw;
		src += H*tw;
		while( H -- ) {
			dst -= tw-W;
			src -= tw-W;
			for( int i = W; i --; )	*--dst = *--src;
		}
	}
}

