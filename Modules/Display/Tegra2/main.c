/**
 * main.c
 * - Driver core
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

// === PROTOTYPES ===
// Driver
 int	Tegra2Vid_Install(char **Arguments);
void	Tegra2Vid_Uninstall();
// Internal
// Filesystem
Uint64	Tegra2Vid_Read(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer);
Uint64	Tegra2Vid_Write(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer);
 int	Tegra2Vid_IOCtl(tVFS_Node *node, int id, void *data);
// -- Internals
 int	Tegra2Vid_int_SetResolution(int W, int H);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, PL110, Tegra2Vid_Install, NULL, NULL);
tDevFS_Driver	gTegra2Vid_DriverStruct = {
	NULL, "PL110",
	{
	.Read = Tegra2Vid_Read,
	.Write = Tegra2Vid_Write,
	.IOCtl = Tegra2Vid_IOCtl
	}
};
// -- Options
tPAddr	gTegra2Vid_PhysBase = Tegra2Vid_BASE;
 int	gbTegra2Vid_IsVersatile = 1;
// -- KeyVal parse rules
const tKeyVal_ParseRules	gTegra2Vid_KeyValueParser = {
	NULL,
	{
		{"Base", "P", &gTegra2Vid_PhysBase},
		{"IsVersatile", "i", &gbTegra2Vid_IsVersatile},
		{NULL, NULL, NULL}
	}
};
// -- Driver state
 int	giTegra2Vid_CurrentMode = 0;
 int	giTegra2Vid_BufferMode;
 int	giTegra2Vid_Width = 640;
 int	giTegra2Vid_Height = 480;
size_t	giTegra2Vid_FramebufferSize;
Uint8	*gpTegra2Vid_IOMem;
tPAddr	gTegra2Vid_FramebufferPhys;
void	*gpTegra2Vid_Framebuffer;
// -- Misc
tDrvUtil_Video_BufInfo	gTegra2Vid_DrvUtil_BufInfo;
tVideo_IOCtl_Pos	gTegra2Vid_CursorPos;

// === CODE ===
/**
 */
int Tegra2Vid_Install(char **Arguments)
{
//	KeyVal_Parse(&gTegra2Vid_KeyValueParser, Arguments);
	
	gpTegra2Vid_IOMem = (void*)MM_MapHWPages(gTegra2Vid_PhysBase, 1);

	Tegra2Vid_int_SetResolution(caTegra2Vid_Modes[0].W, caTegra2Vid_Modes[0].H);

	DevFS_AddDevice( &gTegra2Vid_DriverStruct );

	return 0;
}

/**
 * \brief Clean up resources for driver unloading
 */
void Tegra2Vid_Uninstall()
{
}

/**
 * \brief Read from the framebuffer
 */
Uint64 Tegra2Vid_Read(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer)
{
	return 0;
}

/**
 * \brief Write to the framebuffer
 */
Uint64 Tegra2Vid_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	gTegra2Vid_DrvUtil_BufInfo.BufferFormat = giTegra2Vid_BufferMode;
	return DrvUtil_Video_WriteLFB(&gTegra2Vid_DrvUtil_BufInfo, Offset, Length, Buffer);
}

const char *csaTegra2Vid_IOCtls[] = {DRV_IOCTLNAMES, DRV_VIDEO_IOCTLNAMES, NULL};

/**
 * \brief Handle messages to the device
 */
int Tegra2Vid_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	 int	ret = -2;
	ENTER("pNode iID pData", Node, ID, Data);
	
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_VIDEO, "PL110", VERSION, csaTegra2Vid_IOCtls);

	case VIDEO_IOCTL_SETBUFFORMAT:
		DrvUtil_Video_RemoveCursor( &gTegra2Vid_DrvUtil_BufInfo );
		ret = giTegra2Vid_BufferMode;
		if(Data)	giTegra2Vid_BufferMode = *(int*)Data;
		if(gTegra2Vid_DrvUtil_BufInfo.BufferFormat == VIDEO_BUFFMT_TEXT)
			DrvUtil_Video_SetCursor( &gTegra2Vid_DrvUtil_BufInfo, &gDrvUtil_TextModeCursor );
		break;
	
	case VIDEO_IOCTL_GETSETMODE:
		if(Data)
		{
			 int	newMode;
			
			if( !CheckMem(Data, sizeof(int)) )
				LEAVE_RET('i', -1);
			
			newMode = *(int*)Data;
			
			if(newMode < 0 || newMode >= ciTegra2Vid_ModeCount)
				LEAVE_RET('i', -1);

			if(newMode != giTegra2Vid_CurrentMode)
			{
				giTegra2Vid_CurrentMode = newMode;
				Tegra2Vid_int_SetResolution( caTegra2Vid_Modes[newMode].W, caTegra2Vid_Modes[newMode].H );
			}
		}
		ret = giTegra2Vid_CurrentMode;
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

		for( int i = 0; i < ciTegra2Vid_ModeCount; i ++ )
		{
			 int	area;
			if(mode->width == caTegra2Vid_Modes[i].W && mode->height == caTegra2Vid_Modes[i].H) {
				mode->id = i;
				ret = 1;
				break;
			}
			
			area = caTegra2Vid_Modes[i].W * caTegra2Vid_Modes[i].H;
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
		mode->width = caTegra2Vid_Modes[mode->id].W;
		mode->height = caTegra2Vid_Modes[mode->id].H;
		break;
		}
	
	case VIDEO_IOCTL_MODEINFO:
		{
		tVideo_IOCtl_Mode *mode = Data;
		if(!Data || !CheckMem(Data, sizeof(tVideo_IOCtl_Mode)))
			LEAVE_RET('i', -1);
		if(mode->id < 0 || mode->id >= ciTegra2Vid_ModeCount)
			LEAVE_RET('i', 0);
		

		mode->bpp = 32;
		mode->flags = 0;
		mode->width = caTegra2Vid_Modes[mode->id].W;
		mode->height = caTegra2Vid_Modes[mode->id].H;

		ret = 1;
		break;
		}
	
	case VIDEO_IOCTL_SETCURSOR:
		if( !Data || !CheckMem(Data, sizeof(tVideo_IOCtl_Pos)) )
			LEAVE_RET('i', -1);

		DrvUtil_Video_RemoveCursor( &gTegra2Vid_DrvUtil_BufInfo );
		
		gTegra2Vid_CursorPos = *(tVideo_IOCtl_Pos*)Data;
		if(gTegra2Vid_DrvUtil_BufInfo.BufferFormat == VIDEO_BUFFMT_TEXT)
			DrvUtil_Video_DrawCursor(
				&gTegra2Vid_DrvUtil_BufInfo,
				gTegra2Vid_CursorPos.x*giVT_CharWidth,
				gTegra2Vid_CursorPos.y*giVT_CharHeight
				);
		else
			DrvUtil_Video_DrawCursor(
				&gTegra2Vid_DrvUtil_BufInfo,
				gTegra2Vid_CursorPos.x,
				gTegra2Vid_CursorPos.y
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
int Tegra2Vid_int_SetResolution(int W, int H)
{
	W = (W + 15) & ~0xF;
	if(W <= 0 || H <= 0) {
		Log_Warning("PL110", "Attempted to set invalid resolution (%ix%i)", W, H);
		return 1;
	}
	if(W > 1024)	W = 1920;
	if(H > 768)	H = 1080;

	gpTegra2Vid_IOMem->DCWinAPos = 0;
	gpTegra2Vid_IOMem->DCWinASize = (H << 16) | W;

	if( gpTegra2Vid_Framebuffer ) {
		MM_UnmapHWPages((tVAddr)gpTegra2Vid_Framebuffer, (giTegra2Vid_FramebufferSize+0xFFF)>>12);
	}
	giTegra2Vid_FramebufferSize = W*H*4;

	gpTegra2Vid_Framebuffer = (void*)MM_AllocDMA( (giTegra2Vid_FramebufferSize+0xFFF)>>12, 32, &gTegra2Vid_FramebufferPhys );
	gpTegra2Vid_IOMem->LCDUPBase = gTegra2Vid_FramebufferPhys;
	gpTegra2Vid_IOMem->LCDLPBase = 0;

	// Power on, BGR mode, ???, ???, enabled
	Uint32	controlWord = (1 << 11)|(1 << 8)|(1 << 5)|(5 << 1)|1;
	// According to qemu, the Versatile version has these two the wrong
	// way around
	if( gbTegra2Vid_IsVersatile )
	{
		gpTegra2Vid_IOMem->LCDIMSC = controlWord;	// Actually LCDControl
		gpTegra2Vid_IOMem->LCDControl = 0;	// Actually LCDIMSC
	}
	else
	{
		gpTegra2Vid_IOMem->LCDIMSC = 0;
		gpTegra2Vid_IOMem->LCDControl = controlWord;
	}

	giTegra2Vid_Width = W;
	giTegra2Vid_Height = H;

	// Update the DrvUtil buffer info
	gTegra2Vid_DrvUtil_BufInfo.Framebuffer = gpTegra2Vid_Framebuffer;
	gTegra2Vid_DrvUtil_BufInfo.Pitch = W * 4;
	gTegra2Vid_DrvUtil_BufInfo.Width = W;
	gTegra2Vid_DrvUtil_BufInfo.Height = H;
	gTegra2Vid_DrvUtil_BufInfo.Depth = 32;
	
	return 0;
}

int Tegra2_int_SetMode(int Mode)
{
	*(Uint32*)(gpTegra2Vid_IOMem + DC_DISP_FRONT_PORTCH_0) = (gaTegra2Modes[Mode].VFP << 16) | gaTegra2Modes[Mode].HFP; 
	*(Uint32*)(gpTegra2Vid_IOMem + DC_DISP_SYNC_WIDTH_0) = (gaTegra2Modes[Mode].HS << 16) | gaTegra2Modes[Mode].HS;
	*(Uint32*)(gpTegra2Vid_IOMem + DC_DISP_BACK_PORTCH_0) = (gaTegra2Modes[Mode].VBP << 16) | gaTegra2Modes[Mode].HBP;
	*(Uint32*)(gpTegra2Vid_IOMem + DC_DISP_DISP_ACTIVE_0) = (gaTegra2Modes[Mode].VA << 16) | gaTegra2Modes[Mode].HA;

	*(Uint32*)(gpTegra2Vid_IOMem + DC_WIN_A_POSITION_0) = 0;
	*(Uint32*)(gpTegra2Vid_IOMem + DC_WIN_A_SIZE_0) = (gaTegra2Modes[Mode].VA << 16) | gaTegra2Modes[Mode].HA;
	*(Uint8*)(gpTegra2Vid_IOMem + DC_WIN_A_COLOR_DEPTH_0) = 12;	// Could be 13 (BGR/RGB)
	*(Uint32*)(gpTegra2Vid_IOMem + DC_WIN_A_PRESCALED_SIZE_0) = (gaTegra2Modes[Mode].VA << 16) | gaTegra2Modes[Mode].HA;

	return 0;
}
