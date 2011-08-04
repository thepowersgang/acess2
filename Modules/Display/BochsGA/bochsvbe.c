/**
 * \file drv_bochsvbe.c
 * \brief BGA (Bochs Graphic Adapter) Driver for Acess2
 * \warning This driver does NOT support the Bochs PCI VGA driver
*/
#define DEBUG	0
#include <acess.h>
#include <errno.h>
#include <modules.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <tpl_drv_video.h>

#define INT

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
Uint64	BGA_Read(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer);
Uint64	BGA_Write(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer);
 int	BGA_Ioctl(tVFS_Node *node, int id, void *data);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0032, BochsGA, BGA_Install, NULL, "PCI", NULL);
tDevFS_Driver	gBGA_DriverStruct = {
	NULL, "BochsGA",
	{
	.Read = BGA_Read,
	.Write = BGA_Write,
	.IOCtl = BGA_Ioctl
	}
};
 int	giBGA_CurrentMode = -1;
 int	giBGA_BufferFormat = 0;
tVideo_IOCtl_Pos	gBGA_CursorPos = {-1,-1};
Uint	*gBGA_Framebuffer;
const tBGA_Mode	*gpBGA_CurrentMode;
const tBGA_Mode	gBGA_Modes[] = {
	{640,480,32, 640*480*4},
	{800,600,32, 800*600*4},
	{1024,768,32, 1024*768*4}
};
#define	BGA_MODE_COUNT	(sizeof(gBGA_Modes)/sizeof(gBGA_Modes[0]))

// === CODE ===
/**
 * \fn int BGA_Install(char **Arguments)
 */
int BGA_Install(char **Arguments)
{
	 int	version = 0;
	
	// Check BGA Version
	version = BGA_int_ReadRegister(VBE_DISPI_INDEX_ID);
	
	// NOTE: This driver was written for 0xB0C4, but they seem to be backwards compatable
	if(version != 0xB0C0 && (version < 0xB0C4 || version > 0xB0C5)) {
		Log_Warning("BGA", "Bochs Adapter Version is not 0xB0C4 or 0xB0C5, instead 0x%x", version);
		return MODULE_ERR_NOTNEEDED;
	}
	
	// Install Device
	if(DevFS_AddDevice( &gBGA_DriverStruct ) == -1) {
		Log_Warning("BGA", "Unable to register with DevFS, maybe already loaded?");
		return MODULE_ERR_MISC;
	}
	
	// Map Framebuffer to hardware address
	gBGA_Framebuffer = (void *) MM_MapHWPages(VBE_DISPI_LFB_PHYSICAL_ADDRESS, 768);	// 768 pages (3Mb)
	
	return MODULE_ERR_OK;
}

/**
 * \fn void BGA_Uninstall()
 */
void BGA_Uninstall()
{
	DevFS_DelDevice( &gBGA_DriverStruct );
	MM_UnmapHWPages( VBE_DISPI_LFB_PHYSICAL_ADDRESS, 768 );
}

/**
 * \fn Uint64 BGA_Read(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer)
 * \brief Read from the framebuffer
 */
Uint64 BGA_Read(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer)
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
 * \fn Uint64 BGA_Write(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer)
 * \brief Write to the framebuffer
 */
Uint64 BGA_Write(tVFS_Node *node, Uint64 off, Uint64 len, void *Buffer)
{	
	ENTER("xoff xlen", off, len);
	
	// Check Mode
	if(giBGA_CurrentMode == -1) {
		Log_Notice("BGA", "Setting video mode to #0 (640x480x32)");
		BGA_int_UpdateMode(0);	// Mode Zero is 640x480
	}
	
	// Text Mode
	switch( giBGA_BufferFormat )
	{
	case VIDEO_BUFFMT_TEXT:
		{
		tVT_Char	*chars = Buffer;
		 int	x, y;	// Characters/Rows
		 int	widthInChars = gpBGA_CurrentMode->width/giVT_CharWidth;
		Uint32	*dest;
		
		off /= sizeof(tVT_Char);
		len /= sizeof(tVT_Char);
		
		x = (off % widthInChars);
		y = (off / widthInChars);
		
		// Sanity Check
		if(y > gpBGA_CurrentMode->height / giVT_CharHeight) {
			LEAVE('i', 0);
			return 0;
		}
		
		dest = (Uint32 *)gBGA_Framebuffer;
		dest += y * gpBGA_CurrentMode->width * giVT_CharHeight;
		while(len--)
		{
			VT_Font_Render(
				chars->Ch,
				dest + x*giVT_CharWidth, 32, gpBGA_CurrentMode->width*4,
				VT_Colour12to24(chars->BGCol),
				VT_Colour12to24(chars->FGCol)
				);
			
			chars ++;
			x ++;
			if( x >= widthInChars ) {
				x = 0;
				y ++;	// Why am I keeping track of this?
				dest += gpBGA_CurrentMode->width*giVT_CharHeight;
			}
		}
		}
		break;
	
	case VIDEO_BUFFMT_FRAMEBUFFER:
		{
		Uint8	*destBuf = (Uint8*) ((Uint)gBGA_Framebuffer + (Uint)off);
		
		if( off + len > gpBGA_CurrentMode->fbSize ) {
			LEAVE('i', 0);
			return 0;
		}
		
		LOG("buffer = %p", Buffer);
		LOG("Updating Framebuffer (%p to %p)", destBuf, destBuf + (Uint)len);
		
		
		// Copy to Frambuffer
		memcpy(destBuf, Buffer, len);
		
		LOG("BGA Framebuffer updated");
		}
		break;
	default:
		LEAVE('i', -1);
		return -1;
	}
	
	LEAVE('i', len);
	return len;
}

/**
 * \fn int BGA_Ioctl(tVFS_Node *Node, int ID, void *Data)
 * \brief Handle messages to the device
 */
int BGA_Ioctl(tVFS_Node *Node, int ID, void *Data)
{
	 int	ret = -2;
	ENTER("pNode iId pData", Node, ID, Data);
	
	switch(ID)
	{
	case DRV_IOCTL_TYPE:
		ret = DRV_TYPE_VIDEO;
		break;
	case DRV_IOCTL_IDENT:
		memcpy(Data, "BGA1", 4);
		ret = 1;
		break;
	case DRV_IOCTL_VERSION:
		ret = 0x100;
		break;
	case DRV_IOCTL_LOOKUP:	// TODO: Implement
		ret = 0;
		break;
		
	case VIDEO_IOCTL_GETSETMODE:
		if( Data )
			ret = BGA_int_UpdateMode(*(int*)(Data));
		else
			ret = giBGA_CurrentMode;
		break;
	
	case VIDEO_IOCTL_FINDMODE:
		ret = BGA_int_FindMode((tVideo_IOCtl_Mode*)Data);
		break;
	
	case VIDEO_IOCTL_MODEINFO:
		ret = BGA_int_ModeInfo((tVideo_IOCtl_Mode*)Data);
		break;
	
	case VIDEO_IOCTL_SETBUFFORMAT:
		ret = giBGA_BufferFormat;
		if(Data)
			giBGA_BufferFormat = *(int*)Data;
		break;
	
	case VIDEO_IOCTL_SETCURSOR:
		gBGA_CursorPos.x = ((tVideo_IOCtl_Pos*)Data)->x;
		gBGA_CursorPos.y = ((tVideo_IOCtl_Pos*)Data)->y;
		break;
	
	// Request Access to LFB
//	case VIDEO_IOCTL_REQLFB:
//		ret = BGA_int_MapFB( *(void**)Data );
//		break;
	
	default:
		LEAVE('i', -2);
		return -2;
	}
	
	LEAVE('i', ret);
	return ret;
}

//== Internal Functions ==
/**
 * \fn void BGA_int_WriteRegister(Uint16 reg, Uint16 value)
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

#if 0
void BGA_int_SetBank(Uint16 bank)
{
	BGA_int_WriteRegister(VBE_DISPI_INDEX_BANK, bank);
}
#endif

/**
 * \fn void BGA_int_SetMode(Uint16 width, Uint16 height, Uint16 bpp)
 * \brief Sets the video mode from the dimensions and bpp given
 */
void BGA_int_SetMode(Uint16 Width, Uint16 Height)
{
	ENTER("iWidth iheight ibpp", Width, Height, bpp);
	BGA_int_WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    BGA_int_WriteRegister(VBE_DISPI_INDEX_XRES,	Width);
    BGA_int_WriteRegister(VBE_DISPI_INDEX_YRES,	Height);
    BGA_int_WriteRegister(VBE_DISPI_INDEX_BPP,	32);
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
	 int	factor, tmp;
	 int	rqdProduct = info->width * info->height * info->bpp;
	
	ENTER("pinfo", info);
	LOG("info = {width:%i,height:%i,bpp:%i})\n", info->width, info->height, info->bpp);
	
	for(i = 0; i < BGA_MODE_COUNT; i++)
	{
		#if DEBUG >= 2
		LogF("Mode %i (%ix%ix%i), ", i, gBGA_Modes[i].width, gBGA_Modes[i].height, gBGA_Modes[i].bpp);
		#endif
		
		// Ooh! A perfect match
		if(gBGA_Modes[i].width == info->width
		&& gBGA_Modes[i].height == info->height
		&& gBGA_Modes[i].bpp == info->bpp)
		{
			#if DEBUG >= 2
			LogF("Perfect!\n");
			#endif
			best = i;
			break;
		}
		
		// If not, how close are we?
		tmp = gBGA_Modes[i].width * gBGA_Modes[i].height * gBGA_Modes[i].bpp;
		tmp -= rqdProduct;
		tmp = tmp < 0 ? -tmp : tmp;	// tmp = ABS(tmp)
		factor = tmp * 100 / rqdProduct;
		
		#if DEBUG >= 2
		LogF("factor = %i\n", factor);
		#endif
		
		if(factor < bestFactor)
		{
			bestFactor = factor;
			best = i;
		}
	}
	
	info->id = best;
	info->width = gBGA_Modes[best].width;
	info->height = gBGA_Modes[best].height;
	info->bpp = gBGA_Modes[best].bpp;
	
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

/**
 * \fn int BGA_int_MapFB(void *Dest)
 * \brief Map the framebuffer into a process's space
 * \param Dest	User address to load to
 */
int BGA_int_MapFB(void *Dest)
{
	Uint	i;
	Uint	pages;
	
	// Sanity Check
	if((Uint)Dest > 0xC0000000)	return 0;
	if(gpBGA_CurrentMode->bpp < 15)	return 0;	// Only non-pallete modes are supported
	
	// Count required pages
	pages = (gpBGA_CurrentMode->fbSize + 0xFFF) >> 12;
	
	// Check if there is space
	for( i = 0; i < pages; i++ )
	{
		if(MM_GetPhysAddr( (Uint)Dest + (i << 12) ))
			return 0;
	}
	
	// Map
	for( i = 0; i < pages; i++ )
		MM_Map( (Uint)Dest + (i<<12), VBE_DISPI_LFB_PHYSICAL_ADDRESS + (i<<12) );
	
	return 1;
}
