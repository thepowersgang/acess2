/**
 * \file drv_bochsvbe.c
 * \brief BGA (Bochs Graphic Adapter) Driver
 * \note for AcessOS Version 1
 * \warning This driver does NOT support the Bochs PCI VGA driver
*/
#include <common.h>
#include <modules.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <tpl_drv_video.h>

#define DEBUG	0
#if DEBUG
# define DEBUGS(v...)	SysDebug(v)
#else
# define DEBUGS(v...)
#endif

//#define INT	static
#define INT

// === TYPEDEFS ===
typedef struct {
	Uint16	width;
	Uint16	height;
	Uint16	bpp;
	Uint16	flags;
	Uint32	fbSize;
} t_bga_mode;


// === PROTOTYPES ===
// Driver
 int	BGA_Install(char **Arguments);
void	BGA_Uninstall();
// Internal
void	BGA_int_WriteRegister(Uint16 reg, Uint16 value);
Uint16	BGA_int_ReadRegister(Uint16 reg);
void	BGA_int_SetBank(Uint16 bank);
void	BGA_int_SetMode(Uint16 width, Uint16 height, Uint16 bpp);
 int	BGA_int_UpdateMode(int id);
 int	BGA_int_FindMode(tVideo_IOCtl_Mode *info);
 int	BGA_int_ModeInfo(tVideo_IOCtl_Mode *info);
 int	BGA_int_MapFB(void *Dest);
// Filesystem
Uint64	BGA_Read(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer);
Uint64	BGA_Write(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer);
 int	BGA_Ioctl(tVFS_Node *node, int id, void *data);

// === CONSTANTS ===
const t_bga_mode	BGA_MODES[] = {
	{},
	{640,480,8, 0, 640*480},
	{640,480,32, 0, 640*480*4},
	{800,600,8, 0, 800*600},
	{800,600,32, 0, 800*600*4},
};
#define	BGA_MODE_COUNT	(sizeof(BGA_MODES)/sizeof(BGA_MODES[0]))
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

// GLOBALS
MODULE_DEFINE(0, 0x0032, BochsVBE, BGA_Install, NULL, NULL);
tDevFS_Driver	gBGA_DriverStruct = {
	NULL, "BochsGA",
	{
	.Read = BGA_Read,
	.Write = BGA_Write,
	.IOCtl = BGA_Ioctl
	}
};
 int	giBGA_CurrentMode = -1;
 int	giBGA_DriverId = -1;
Uint	*gBGA_Framebuffer;

// === CODE ===
/**
 * \fn int BGA_Install(char **Arguments)
 */
int BGA_Install(char **Arguments)
{
	 int	bga_version = 0;
	
	// Check BGA Version
	bga_version = BGA_int_ReadRegister(VBE_DISPI_INDEX_ID);
	// NOTE: This driver was written for 0xB0C4, but they seem to be backwards compatable
	if(bga_version < 0xB0C4 || bga_version > 0xB0C5) {
		Warning("[BGA ] Bochs Adapter Version is not 0xB0C4 or 0xB0C5, instead 0x%x", bga_version);
		return 0;
	}
	
	// Install Device
	giBGA_DriverId = DevFS_AddDevice( &gBGA_DriverStruct );
	if(giBGA_DriverId == -1) {
		Warning("[BGA ] Unable to register with DevFS, maybe already loaded?");
		return 0;
	}
	
	// Map Framebuffer to hardware address
	gBGA_Framebuffer = (void *) MM_MapHWPage(VBE_DISPI_LFB_PHYSICAL_ADDRESS, 768);	// 768 pages (3Mb)
	
	return 1;
}

/**
 * \fn void BGA_Uninstall()
 */
void BGA_Uninstall()
{
	//DevFS_DelDevice( giBGA_DriverId );
	MM_UnmapHWPage( VBE_DISPI_LFB_PHYSICAL_ADDRESS, 768 );
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
	if(off+len > BGA_MODES[giBGA_CurrentMode].fbSize)
		return -1;
	
	// Copy from Framebuffer
	memcpy(buffer, (void*)((Uint)gBGA_Framebuffer + (Uint)off), len);
	return len;
}

/**
 * \fn Uint64 BGA_Write(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer)
 * \brief Write to the framebuffer
 */
Uint64 BGA_Write(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer)
{
	Uint8	*destBuf;
	
	DEBUGS("BGA_Write: (off=%i, len=0x%x)\n", off, len);
	
	// Check Mode
	if(giBGA_CurrentMode == -1)
		return -1;
	// Check Input against Frambuffer Size
	if(off+len > BGA_MODES[giBGA_CurrentMode].fbSize)
		return -1;
	
	destBuf = (Uint8*) ((Uint)gBGA_Framebuffer + (Uint)off);
	
	DEBUGS(" BGA_Write: *buffer = 0x%x\n", *(Uint*)buffer);
	DEBUGS(" BGA_Write: Updating Framebuffer (0x%x - 0x%x bytes)\n", 
		destBuf, destBuf + (Uint)len);
	
	
	// Copy to Frambuffer
	memcpy(destBuf, buffer, len);
	
	DEBUGS("BGA_Write: BGA Framebuffer updated\n");
	
	return len;
}

/**
 * \fn INT int BGA_Ioctl(tVFS_Node *node, int id, void *data)
 * \brief Handle messages to the device
 */
INT int BGA_Ioctl(tVFS_Node *node, int id, void *data)
{
	 int	ret = -2;
	ENTER("pNode iId pData", node, id, data);
	
	switch(id)
	{
	case DRV_IOCTL_TYPE:
		ret = DRV_TYPE_VIDEO;
		break;
	case DRV_IOCTL_IDENT:
		memcpy(data, "BGA1", 4);
		ret = 1;
		break;
	case DRV_IOCTL_VERSION:
		ret = 0x100;
		break;
	case DRV_IOCTL_LOOKUP:	// TODO: Implement
		ret = 0;
		break;
		
	case VIDEO_IOCTL_SETMODE:
		ret = BGA_int_UpdateMode(*(int*)(data));
		break;
		
	case VIDEO_IOCTL_GETMODE:
		ret = giBGA_CurrentMode;
		break;
	
	case VIDEO_IOCTL_FINDMODE:
		ret = BGA_int_FindMode((tVideo_IOCtl_Mode*)data);
		break;
	
	case VIDEO_IOCTL_MODEINFO:
		ret = BGA_int_ModeInfo((tVideo_IOCtl_Mode*)data);
		break;
	
	// Request Access to LFB
	case VIDEO_IOCTL_REQLFB:
		ret = BGA_int_MapFB( *(void**)data );
		break;
	
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

INT Uint16 BGA_int_ReadRegister(Uint16 reg)
{
	outw(VBE_DISPI_IOPORT_INDEX, reg);
	return inw(VBE_DISPI_IOPORT_DATA);
}

#if 0
INT void BGA_int_SetBank(Uint16 bank)
{
	BGA_int_WriteRegister(VBE_DISPI_INDEX_BANK, bank);
}
#endif

/**
 * \fn void BGA_int_SetMode(Uint16 width, Uint16 height, Uint16 bpp)
 * \brief Sets the video mode from the dimensions and bpp given
 */
void BGA_int_SetMode(Uint16 width, Uint16 height, Uint16 bpp)
{
	DEBUGS("BGA_int_SetMode: (width=%i, height=%i, bpp=%i)\n", width, height, bpp);
	BGA_int_WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    BGA_int_WriteRegister(VBE_DISPI_INDEX_XRES,	width);
    BGA_int_WriteRegister(VBE_DISPI_INDEX_YRES,	height);
    BGA_int_WriteRegister(VBE_DISPI_INDEX_BPP,	bpp);
    BGA_int_WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_NOCLEARMEM | VBE_DISPI_LFB_ENABLED);
    //BGA_int_WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_NOCLEARMEM);
}

/**
 * \fn int BGA_int_UpdateMode(int id)
 * \brief Set current vide mode given a mode id
 */
int BGA_int_UpdateMode(int id)
{
	if(id < 0 || id >= BGA_MODE_COUNT)	return -1;
	BGA_int_SetMode(BGA_MODES[id].width, BGA_MODES[id].height, BGA_MODES[id].bpp);
	giBGA_CurrentMode = id;
	return id;
}

/**
 * \fn int BGA_int_FindMode(tVideo_IOCtl_Mode *info)
 * \brief Find a mode matching the given options
 */
int BGA_int_FindMode(tVideo_IOCtl_Mode *info)
{
	 int	i;
	 int	best = -1, bestFactor = 1000;
	 int	factor, tmp;
	 int	rqdProduct = info->width * info->height * info->bpp;
	
	DEBUGS("BGA_int_FindMode: (info={width:%i,height:%i,bpp:%i})\n", info->width, info->height, info->bpp);
	
	for(i = 0; i < BGA_MODE_COUNT; i++)
	{
		#if DEBUG >= 2
		LogF("Mode %i (%ix%ix%i), ", i, BGA_MODES[i].width, BGA_MODES[i].height, BGA_MODES[i].bpp);
		#endif
	
		if(BGA_MODES[i].width == info->width
		&& BGA_MODES[i].height == info->height
		&& BGA_MODES[i].bpp == info->bpp)
		{
			#if DEBUG >= 2
			LogF("Perfect!\n");
			#endif
			best = i;
			break;
		}
		
		tmp = BGA_MODES[i].width * BGA_MODES[i].height * BGA_MODES[i].bpp;
		tmp -= rqdProduct;
		tmp = tmp < 0 ? -tmp : tmp;
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
	info->width = BGA_MODES[best].width;
	info->height = BGA_MODES[best].height;
	info->bpp = BGA_MODES[best].bpp;
	return best;
}

/**
 * \fn int BGA_int_ModeInfo(tVideo_IOCtl_Mode *info)
 * \brief Get mode information
 */
int BGA_int_ModeInfo(tVideo_IOCtl_Mode *info)
{
	if(!info)	return -1;
	if(MM_GetPhysAddr((Uint)info) == 0)
		return -1;
	
	if(info->id < 0 || info->id >= BGA_MODE_COUNT)	return -1;
	
	info->width = BGA_MODES[info->id].width;
	info->height = BGA_MODES[info->id].height;
	info->bpp = BGA_MODES[info->id].bpp;
	
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
	if(BGA_MODES[giBGA_CurrentMode].bpp < 15)	return 0;	// Only non-pallete modes are supported
	
	// Count required pages
	pages = (BGA_MODES[giBGA_CurrentMode].fbSize + 0xFFF) >> 12;
	
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
