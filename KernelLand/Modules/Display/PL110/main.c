/**
 * Acess2 ARM PrimeCell Colour LCD Controller (PL110) Driver
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Driver core
 *
 *
 * NOTE: The PL110 is set to 24bpp, but these are stored as 32-bit words.
 *       This corresponds to the Acess 32bpp mode, as the Acess 24bpp is packed
 */
#define DEBUG	0
#define VERSION	((0<<8)|10)
#include <acess.h>
#include <errno.h>
#include <modules.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <api_drv_video.h>
#include <lib/keyvalue.h>
#include <options.h>	// ARM Arch

#define ABS(a)	((a)>0?(a):-(a))

// === TYPEDEFS ===
typedef struct sPL110	tPL110;

struct sPL110
{
	Uint32	LCDTiming0;
	Uint32	LCDTiming1;
	Uint32	LCDTiming2;
	Uint32	LCDTiming3;
	
	Uint32	LCDUPBase;
	Uint32	LCDLPBase;
	Uint32	LCDIMSC;
	Uint32	LCDControl;
	Uint32	LCDRIS;
	Uint32	LCDMIS;
	Uint32	LCDICR;
	Uint32	LCDUPCurr;
	Uint32	LCDLPCurr;
};

#ifndef PL110_BASE
#define PL110_BASE	0x10020000	// Integrator
#endif

// === CONSTANTS ===
const struct {
	short W, H;
}	caPL110_Modes[] = {
	{640,480},
	{800,600},
	{1024,768}	// MAX
};
const int	ciPL110_ModeCount = sizeof(caPL110_Modes)/sizeof(caPL110_Modes[0]);

// === PROTOTYPES ===
// Driver
 int	PL110_Install(char **Arguments);
void	PL110_Uninstall();
// Internal
// Filesystem
size_t	PL110_Read(tVFS_Node *node, off_t Offset, size_t len, void *buffer);
size_t	PL110_Write(tVFS_Node *node, off_t offset, size_t len, const void *buffer);
 int	PL110_IOCtl(tVFS_Node *node, int id, void *data);
// -- Internals
 int	PL110_int_SetResolution(int W, int H);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, PL110, PL110_Install, NULL, NULL);
tVFS_NodeType	gPL110_DevNodeType = {
	.Read = PL110_Read,
	.Write = PL110_Write,
	.IOCtl = PL110_IOCtl
	};
tDevFS_Driver	gPL110_DriverStruct = {
	NULL, "PL110",
	{.Type = &gPL110_DevNodeType}
};
// -- Options
tPAddr	gPL110_PhysBase = PL110_BASE;
 int	gbPL110_IsVersatile = 1;
// -- KeyVal parse rules
const tKeyVal_ParseRules	gPL110_KeyValueParser = {
	NULL,
	{
		{"Base", "P", &gPL110_PhysBase},
		{"IsVersatile", "i", &gbPL110_IsVersatile},
		{NULL, NULL, NULL}
	}
};
// -- Driver state
 int	giPL110_CurrentMode = 0;
 int	giPL110_BufferMode;
 int	giPL110_Width = 640;
 int	giPL110_Height = 480;
size_t	giPL110_FramebufferSize;
tPL110	*gpPL110_IOMem;
tPAddr	gPL110_FramebufferPhys;
void	*gpPL110_Framebuffer;
// -- Misc
tDrvUtil_Video_BufInfo	gPL110_DrvUtil_BufInfo;
tVideo_IOCtl_Pos	gPL110_CursorPos;

// === CODE ===
/**
 */
int PL110_Install(char **Arguments)
{
//	KeyVal_Parse(&gPL110_KeyValueParser, Arguments);
	
	gpPL110_IOMem = (void*)MM_MapHWPages(gPL110_PhysBase, 1);

	PL110_int_SetResolution(caPL110_Modes[0].W, caPL110_Modes[0].H);

	DevFS_AddDevice( &gPL110_DriverStruct );

	return 0;
}

/**
 * \brief Clean up resources for driver unloading
 */
void PL110_Uninstall()
{
}

/**
 * \brief Read from the framebuffer
 */
size_t PL110_Read(tVFS_Node *node, off_t off, size_t len, void *buffer)
{
	return 0;
}

/**
 * \brief Write to the framebuffer
 */
size_t PL110_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer)
{
	gPL110_DrvUtil_BufInfo.BufferFormat = giPL110_BufferMode;
	return DrvUtil_Video_WriteLFB(&gPL110_DrvUtil_BufInfo, Offset, Length, Buffer);
}

const char *csaPL110_IOCtls[] = {DRV_IOCTLNAMES, DRV_VIDEO_IOCTLNAMES, NULL};

/**
 * \brief Handle messages to the device
 */
int PL110_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	 int	ret = -2;
	ENTER("pNode iID pData", Node, ID, Data);
	
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_VIDEO, "PL110", VERSION, csaPL110_IOCtls);

	case VIDEO_IOCTL_SETBUFFORMAT:
		DrvUtil_Video_RemoveCursor( &gPL110_DrvUtil_BufInfo );
		ret = giPL110_BufferMode;
		if(Data)	giPL110_BufferMode = *(int*)Data;
		if(gPL110_DrvUtil_BufInfo.BufferFormat == VIDEO_BUFFMT_TEXT)
			DrvUtil_Video_SetCursor( &gPL110_DrvUtil_BufInfo, &gDrvUtil_TextModeCursor );
		break;
	
	case VIDEO_IOCTL_GETSETMODE:
		if(Data)
		{
			 int	newMode;
			
			if( !CheckMem(Data, sizeof(int)) )
				LEAVE_RET('i', -1);
			
			newMode = *(int*)Data;
			
			if(newMode < 0 || newMode >= ciPL110_ModeCount)
				LEAVE_RET('i', -1);

			if(newMode != giPL110_CurrentMode)
			{
				giPL110_CurrentMode = newMode;
				PL110_int_SetResolution( caPL110_Modes[newMode].W, caPL110_Modes[newMode].H );
			}
		}
		ret = giPL110_CurrentMode;
		break;
	
	case VIDEO_IOCTL_FINDMODE:
		{
		tVideo_IOCtl_Mode *mode = Data;
		 int	closest, closestArea, reqArea = 0;
		if(!Data || !CheckMem(Data, sizeof(tVideo_IOCtl_Mode)))
			LEAVE_RET('i', -1);
		if( mode->bpp != 32 )
			LEAVE_RET('i', 0);
		if( mode->flags	!= 0 )
			LEAVE_RET('i', 0);

		ret = 0;

		for( int i = 0; i < ciPL110_ModeCount; i ++ )
		{
			 int	area;
			if(mode->width == caPL110_Modes[i].W && mode->height == caPL110_Modes[i].H) {
				mode->id = i;
				ret = 1;
				break;
			}
			
			area = caPL110_Modes[i].W * caPL110_Modes[i].H;
			if(!reqArea) {
				reqArea = mode->width * mode->height;
				closest = i;
				closestArea = area;
			}
			else if( ABS(area - reqArea) < ABS(closestArea - reqArea) ) {
				closest = i;
				closestArea = area;
			}
		}
		
		if( ret == 0 )
		{
			mode->id = closest;
			ret = 1;
		}
		mode->width = caPL110_Modes[mode->id].W;
		mode->height = caPL110_Modes[mode->id].H;
		break;
		}
	
	case VIDEO_IOCTL_MODEINFO:
		{
		tVideo_IOCtl_Mode *mode = Data;
		if(!Data || !CheckMem(Data, sizeof(tVideo_IOCtl_Mode)))
			LEAVE_RET('i', -1);
		if(mode->id < 0 || mode->id >= ciPL110_ModeCount)
			LEAVE_RET('i', 0);
		

		mode->bpp = 32;
		mode->flags = 0;
		mode->width = caPL110_Modes[mode->id].W;
		mode->height = caPL110_Modes[mode->id].H;

		ret = 1;
		break;
		}
	
	case VIDEO_IOCTL_SETCURSOR:
		if( !Data || !CheckMem(Data, sizeof(tVideo_IOCtl_Pos)) )
			LEAVE_RET('i', -1);

		DrvUtil_Video_RemoveCursor( &gPL110_DrvUtil_BufInfo );
		
		gPL110_CursorPos = *(tVideo_IOCtl_Pos*)Data;
		if(gPL110_DrvUtil_BufInfo.BufferFormat == VIDEO_BUFFMT_TEXT)
			DrvUtil_Video_DrawCursor(
				&gPL110_DrvUtil_BufInfo,
				gPL110_CursorPos.x*giVT_CharWidth,
				gPL110_CursorPos.y*giVT_CharHeight
				);
		else
			DrvUtil_Video_DrawCursor(
				&gPL110_DrvUtil_BufInfo,
				gPL110_CursorPos.x,
				gPL110_CursorPos.y
				);
		break;
	
	default:
		LEAVE('i', -2);
		return -2;
	}
	
	LEAVE('i', ret);
	return ret;
}

//
//
//

/**
 * \brief Set the LCD controller resolution
 * \param W	Width (aligned to 16 pixels, cipped to 1024)
 * \param H	Height (clipped to 768)
 * \return Boolean failure
 */
int PL110_int_SetResolution(int W, int H)
{
	W = (W + 15) & ~0xF;
	if(W <= 0 || H <= 0) {
		Log_Warning("PL110", "Attempted to set invalid resolution (%ix%i)", W, H);
		return 1;
	}
	if(W > 1024)	W = 1024;
	if(H > 768)	H = 768;

	gpPL110_IOMem->LCDTiming0 = ((W/16)-1) << 2;
	gpPL110_IOMem->LCDTiming1 = H-1;
	gpPL110_IOMem->LCDTiming2 = (14 << 27);
	gpPL110_IOMem->LCDTiming3 = 0;

	if( gpPL110_Framebuffer ) {
		MM_UnmapHWPages((tVAddr)gpPL110_Framebuffer, (giPL110_FramebufferSize+0xFFF)>>12);
	}
	giPL110_FramebufferSize = W*H*4;

	gpPL110_Framebuffer = (void*)MM_AllocDMA( (giPL110_FramebufferSize+0xFFF)>>12, 32, &gPL110_FramebufferPhys );
	gpPL110_IOMem->LCDUPBase = gPL110_FramebufferPhys;
	gpPL110_IOMem->LCDLPBase = 0;

	// Power on, BGR mode, ???, ???, enabled
	Uint32	controlWord = (1 << 11)|(1 << 8)|(1 << 5)|(5 << 1)|1;
	// According to qemu, the Versatile version has these two the wrong
	// way around
	if( gbPL110_IsVersatile )
	{
		gpPL110_IOMem->LCDIMSC = controlWord;	// Actually LCDControl
		gpPL110_IOMem->LCDControl = 0;	// Actually LCDIMSC
	}
	else
	{
		gpPL110_IOMem->LCDIMSC = 0;
		gpPL110_IOMem->LCDControl = controlWord;
	}

	giPL110_Width = W;
	giPL110_Height = H;

	// Update the DrvUtil buffer info
	gPL110_DrvUtil_BufInfo.Framebuffer = gpPL110_Framebuffer;
	gPL110_DrvUtil_BufInfo.Pitch = W * 4;
	gPL110_DrvUtil_BufInfo.Width = W;
	gPL110_DrvUtil_BufInfo.Height = H;
	gPL110_DrvUtil_BufInfo.Depth = 32;
	
	return 0;
}
