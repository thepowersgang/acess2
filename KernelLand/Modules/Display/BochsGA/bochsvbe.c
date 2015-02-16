/**
 * Acess2 Bochs graphics adapter Driver
 * - By John Hodge (thePowersGang)
 *
 * bochsvbe.c
 * - Driver core
 */
#define DEBUG	0
#define VERSION	VER2(0,10)

#include <acess.h>
#include <errno.h>
#include <modules.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <api_drv_video.h>

// === TYPES ===
typedef struct sBGA_Mode {
	Uint16	width;
	Uint16	height;
	Uint16	bpp;
	Uint32	fbSize;
}	tBGA_Mode;

// === CONSTANTS ===
#define	BGA_LFB_MAXSIZE	(1024*768*4)
#define	VBE_DISPI_BANK_ADDRESS	0xA0000
#define VBE_DISPI_LFB_PHYSICAL_ADDRESS	0xE0000000
#define VBE_DISPI_IOPORT_INDEX	0x01CE
#define	VBE_DISPI_IOPORT_DATA	0x01CF
#define	VBE_DISPI_DISABLED	0x00
#define VBE_DISPI_ENABLED	0x01
#define	VBE_DISPI_LFB_ENABLED	0x40
#define	VBE_DISPI_NOCLEARMEM	0x80
enum {
	VBE_DISPI_INDEX_ID,
	VBE_DISPI_INDEX_XRES,
	VBE_DISPI_INDEX_YRES,
	VBE_DISPI_INDEX_BPP,
	VBE_DISPI_INDEX_ENABLE,
	VBE_DISPI_INDEX_BANK,
	VBE_DISPI_INDEX_VIRT_WIDTH,
	VBE_DISPI_INDEX_VIRT_HEIGHT,
	VBE_DISPI_INDEX_X_OFFSET,
	VBE_DISPI_INDEX_Y_OFFSET
};

// === PROTOTYPES ===
// Driver
 int	BGA_Install(char **Arguments);
void	BGA_Uninstall();
// Internal
void	BGA_int_WriteRegister(Uint16 reg, Uint16 value);
Uint16	BGA_int_ReadRegister(Uint16 reg);
void	BGA_int_SetBank(Uint16 bank);
void	BGA_int_SetMode(Uint16 width, Uint16 height);
 int	BGA_int_UpdateMode(int id);
 int	BGA_int_FindMode(tVideo_IOCtl_Mode *info);
 int	BGA_int_ModeInfo(tVideo_IOCtl_Mode *info);
 int	BGA_int_MapFB(void *Dest);
// Filesystem
size_t	BGA_Read(tVFS_Node *Node, off_t off, size_t len, void *buffer, Uint Flags);
size_t	BGA_Write(tVFS_Node *Node, off_t off, size_t len, const void *buffer, Uint Flags);
 int	BGA_IOCtl(tVFS_Node *Node, int ID, void *Data);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, BochsGA, BGA_Install, NULL, "PCI", NULL);
tVFS_NodeType	gBGA_NodeType = {
	.Read = BGA_Read,
	.Write = BGA_Write,
	.IOCtl = BGA_IOCtl
	};
tDevFS_Driver	gBGA_DriverStruct = {
	NULL, "BochsGA",
	{.Type = &gBGA_NodeType}
	};
 int	giBGA_CurrentMode = -1;
tVideo_IOCtl_Pos	gBGA_CursorPos = {-1,-1};
Uint	*gBGA_Framebuffer;
const tBGA_Mode	*gpBGA_CurrentMode;
const tBGA_Mode	gBGA_Modes[] = {
	{640,480,32, 640*480*4},
	{800,480,32, 800*480*4},	// Nice mode for VM testing
	{800,600,32, 800*600*4},
	{1024,768,32, 1024*768*4}
};
#define	BGA_MODE_COUNT	(sizeof(gBGA_Modes)/sizeof(gBGA_Modes[0]))
tDrvUtil_Video_BufInfo	gBGA_DrvUtil_BufInfo;

// === CODE ===
/**
 * \fn int BGA_Install(char **Arguments)
 */
int BGA_Install(char **Arguments)
{
	 int	version = 0;
	tPAddr	base;
	tPCIDev	dev;
	
	// Check BGA Version
	version = BGA_int_ReadRegister(VBE_DISPI_INDEX_ID);
	LOG("version = 0x%x", version);
	if( version == 0xFFFF ) {
		// Floating bus, nothing there
		return MODULE_ERR_NOTNEEDED;
	}
	
	// NOTE: This driver was written for BGA versions >= 0xBOC2
	// NOTE: However, Qemu is braindead and doesn't return the actual version
	if( version != 0xB0C0 && ((version & 0xFFF0) != 0xB0C0 || version < 0xB0C2) ) {
		Log_Warning("BGA", "Bochs Adapter Version is not compatible (need >= 0xB0C2), instead 0x%x", version);
		return MODULE_ERR_NOTNEEDED;
	}

	// Get framebuffer base	
	dev = PCI_GetDevice(0x1234, 0x1111, 0);
	if(dev == -1)
		base = VBE_DISPI_LFB_PHYSICAL_ADDRESS;
	else {
		Log_Debug("BGA", "BARs %x,%x,%x,%x,%x,%x",
			PCI_GetBAR(dev, 0), PCI_GetBAR(dev, 1), PCI_GetBAR(dev, 2),
			PCI_GetBAR(dev, 3), PCI_GetBAR(dev, 4), PCI_GetBAR(dev, 5));
		base = PCI_GetValidBAR(dev, 0, PCI_BARTYPE_MEM);
		// TODO: Qemu/bochs have MMIO versions of the registers in BAR2
		// - This range is non-indexed
		//mmio_base = PCI_GetValidBAR(dev, 2, PCI_BARTYPE_MEM);
	}

	// Map Framebuffer to hardware address
	gBGA_Framebuffer = (void *) MM_MapHWPages(base, 768);	// 768 pages (3Mb)

	// Install Device
	if( DevFS_AddDevice( &gBGA_DriverStruct ) == -1 )
	{
		Log_Warning("BGA", "Unable to register with DevFS, maybe already loaded?");
		return MODULE_ERR_MISC;
	}
		
	return MODULE_ERR_OK;
}

/**
 * \brief Clean up driver resources before destruction
 */
void BGA_Uninstall(void)
{
	DevFS_DelDevice( &gBGA_DriverStruct );
	MM_UnmapHWPages( gBGA_Framebuffer, 768 );
}

/**
 * \brief Read from the framebuffer
 */
size_t BGA_Read(tVFS_Node *node, off_t off, size_t len, void *buffer, Uint Flags)
{
	// Check Mode
	if(giBGA_CurrentMode == -1)	return -1;
	
	// Check Offset and Length against Framebuffer Size
	if(off+len > gpBGA_CurrentMode->fbSize)
		return -1;
	
	// Copy from Framebuffer
	memcpy(buffer, (void*)((Uint)gBGA_Framebuffer + (Uint)off), len);
	return len;
}

/**
 * \brief Write to the framebuffer
 */
size_t BGA_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags)
{
	if( giBGA_CurrentMode == -1 )	BGA_int_UpdateMode(0);
	return DrvUtil_Video_WriteLFB(&gBGA_DrvUtil_BufInfo, Offset, Length, Buffer);
}

const char *csaBGA_IOCtls[] = {DRV_IOCTLNAMES, DRV_VIDEO_IOCTLNAMES, NULL};
/**
 * \brief Handle messages to the device
 */
int BGA_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	 int	ret = -2;
	ENTER("pNode iId pData", Node, ID, Data);
	
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_VIDEO, "BochsGA", VERSION, csaBGA_IOCtls);

	case VIDEO_IOCTL_GETSETMODE:
		if( Data )	BGA_int_UpdateMode(*(int*)(Data));
		ret = giBGA_CurrentMode;
		break;
	
	case VIDEO_IOCTL_FINDMODE:
		ret = BGA_int_FindMode((tVideo_IOCtl_Mode*)Data);
		break;
	
	case VIDEO_IOCTL_MODEINFO:
		ret = BGA_int_ModeInfo((tVideo_IOCtl_Mode*)Data);
		break;
	
	case VIDEO_IOCTL_SETBUFFORMAT:
		DrvUtil_Video_RemoveCursor( &gBGA_DrvUtil_BufInfo );
		ret = gBGA_DrvUtil_BufInfo.BufferFormat;
		if(Data)
			gBGA_DrvUtil_BufInfo.BufferFormat = *(int*)Data;
		if(gBGA_DrvUtil_BufInfo.BufferFormat == VIDEO_BUFFMT_TEXT)
			DrvUtil_Video_SetCursor( &gBGA_DrvUtil_BufInfo, &gDrvUtil_TextModeCursor );
		break;
	
	case VIDEO_IOCTL_SETCURSOR:
		DrvUtil_Video_RemoveCursor( &gBGA_DrvUtil_BufInfo );
		gBGA_CursorPos.x = ((tVideo_IOCtl_Pos*)Data)->x;
		gBGA_CursorPos.y = ((tVideo_IOCtl_Pos*)Data)->y;
		if(gBGA_DrvUtil_BufInfo.BufferFormat == VIDEO_BUFFMT_TEXT)
			DrvUtil_Video_DrawCursor(
				&gBGA_DrvUtil_BufInfo,
				gBGA_CursorPos.x*giVT_CharWidth,
				gBGA_CursorPos.y*giVT_CharHeight
				);
		else
			DrvUtil_Video_DrawCursor(
				&gBGA_DrvUtil_BufInfo,
				gBGA_CursorPos.x, gBGA_CursorPos.y
				);
		break;

	case VIDEO_IOCTL_SETCURSORBITMAP:
		DrvUtil_Video_SetCursor( &gBGA_DrvUtil_BufInfo, Data );
		return 0;
	
	default:
		LEAVE('i', -2);
		return -2;
	}
	
	LEAVE('i', ret);
	return ret;
}

//== Internal Functions ==
/**
 * \brief Writes to a BGA register
 */
void BGA_int_WriteRegister(Uint16 reg, Uint16 value)
{
	outw(VBE_DISPI_IOPORT_INDEX, reg);
	outw(VBE_DISPI_IOPORT_DATA, value);
}

Uint16 BGA_int_ReadRegister(Uint16 reg)
{
	outw(VBE_DISPI_IOPORT_INDEX, reg);
	return inw(VBE_DISPI_IOPORT_DATA);
}

/**
 * \brief Sets the video mode (32bpp only)
 */
void BGA_int_SetMode(Uint16 Width, Uint16 Height)
{
	ENTER("iWidth iHeight", Width, Height);
	BGA_int_WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
	BGA_int_WriteRegister(VBE_DISPI_INDEX_XRES, Width);
	BGA_int_WriteRegister(VBE_DISPI_INDEX_YRES, Height);
	BGA_int_WriteRegister(VBE_DISPI_INDEX_BPP, 32);
	BGA_int_WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_NOCLEARMEM | VBE_DISPI_LFB_ENABLED);
	LEAVE('-');
}

/**
 * \fn int BGA_int_UpdateMode(int id)
 * \brief Set current vide mode given a mode id
 */
int BGA_int_UpdateMode(int id)
{
	// Sanity Check
	if(id < 0 || id >= BGA_MODE_COUNT)	return -1;
	
	BGA_int_SetMode(
		gBGA_Modes[id].width,
		gBGA_Modes[id].height);
	
	gBGA_DrvUtil_BufInfo.Framebuffer = gBGA_Framebuffer;
	gBGA_DrvUtil_BufInfo.Pitch = gBGA_Modes[id].width * 4;
	gBGA_DrvUtil_BufInfo.Width = gBGA_Modes[id].width;
	gBGA_DrvUtil_BufInfo.Height = gBGA_Modes[id].height;
	gBGA_DrvUtil_BufInfo.Depth = gBGA_Modes[id].bpp;

	giBGA_CurrentMode = id;
	gpBGA_CurrentMode = &gBGA_Modes[id];
	return id;
}

/**
 * \fn int BGA_int_FindMode(tVideo_IOCtl_Mode *info)
 * \brief Find a mode matching the given options
 */
int BGA_int_FindMode(tVideo_IOCtl_Mode *info)
{
	 int	i;
	 int	best = 0, bestFactor = 1000;
	 int	tmp;
	 int	rqdProduct = info->width * info->height;
	
	ENTER("pinfo", info);
	LOG("info = {width:%i,height:%i,bpp:%i})\n", info->width, info->height, info->bpp);
	
	for(i = 0; i < BGA_MODE_COUNT; i++)
	{
		#if DEBUG >= 2
		LOG("Mode %i (%ix%i,%ibpp), ", i, gBGA_Modes[i].width, gBGA_Modes[i].height, gBGA_Modes[i].bpp);
		#endif

		if( gBGA_Modes[i].bpp != info->bpp )
			continue ;		
		
		// Ooh! A perfect match
		if(gBGA_Modes[i].width == info->width && gBGA_Modes[i].height == info->height)
		{
			#if DEBUG >= 2
			LOG("Perfect");
			#endif
			best = i;
			break;
		}


		// If not, how close are we?
		tmp = gBGA_Modes[i].width * gBGA_Modes[i].height - rqdProduct;
		tmp = tmp < 0 ? -tmp : tmp;	// tmp = ABS(tmp)
		
		#if DEBUG >= 2
		LOG("tmp = %i", tmp);
		#endif
		
		if(tmp < bestFactor)
		{
			bestFactor = tmp;
			best = i;
		}
	}
	
	info->id = best;
	info->width = gBGA_Modes[best].width;
	info->height = gBGA_Modes[best].height;
	info->bpp = gBGA_Modes[best].bpp;

	LEAVE('i', best);	
	return best;
}

/**
 * \fn int BGA_int_ModeInfo(tVideo_IOCtl_Mode *info)
 * \brief Get mode information
 */
int BGA_int_ModeInfo(tVideo_IOCtl_Mode *info)
{
	// Sanity Check
	//if( !MM_IsUser( (Uint)info, sizeof(tVideo_IOCtl_Mode) ) ) {
	//	return -EINVAL;
	//}
	
	if(info->id < 0 || info->id >= BGA_MODE_COUNT)	return -1;
	
	info->width = gBGA_Modes[info->id].width;
	info->height = gBGA_Modes[info->id].height;
	info->bpp = gBGA_Modes[info->id].bpp;
	
	return 1;
}

