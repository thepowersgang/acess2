/*
 * Acess2 VGA Controller Driver
 */
#define DEBUG	0
#include <acess.h>
#include <fs_devfs.h>
#include <tpl_drv_video.h>
#include <modules.h>

// === CONSTANTS ===
#define	VGA_WIDTH	80
#define	VGA_HEIGHT	25

// === PROTOTYPES ===
 int	VGA_Install(char **Arguments);
Uint64	VGA_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	VGA_IOCtl(tVFS_Node *Node, int Id, void *Data);
Uint16	VGA_int_GetWord(tVT_Char *Char);
void	VGA_int_SetCursor(Sint16 x, Sint16 y);

// === GLOBALS ===
MODULE_DEFINE(0, 0x000A, VGA, VGA_Install, NULL, NULL);
tDevFS_Driver	gVGA_DevInfo = {
	NULL, "VGA",
	{
	.NumACLs = 0,
	.Size = VGA_WIDTH*VGA_HEIGHT*sizeof(tVT_Char),
	//.Read = VGA_Read,
	.Write = VGA_Write,
	.IOCtl = VGA_IOCtl
	}
};
Uint16	*gVGA_Framebuffer = (void*)( KERNEL_BASE|0xB8000 );

// === CODE ===
/**
 * \fn int VGA_Install(char **Arguments)
 */
int VGA_Install(char **Arguments)
{
	Uint8	byte;
	
	// Enable Bright Backgrounds
	inb(0x3DA);	// Reset flipflop
	outb(0x3C0, 0x30);	// Index 0x10, PAS
	byte = inb(0x3C1);
	byte &= ~8;	// Disable Blink
	outb(0x3C0, byte);	// Write value
	
	
	// Install DevFS
	DevFS_AddDevice( &gVGA_DevInfo );
	
	return MODULE_ERR_OK;
}

/**
 * \fn Uint64 VGA_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Writes a string of bytes to the VGA controller
 */
Uint64 VGA_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	 int	num = Length / sizeof(tVT_Char);
	 int	ofs = Offset / sizeof(tVT_Char);
	 int	i = 0;
	tVT_Char	*chars = Buffer;
	Uint16	word;
	
	//ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);
	
	for( ; num--; i ++, ofs ++)
	{
		word = VGA_int_GetWord( &chars[i] );
		gVGA_Framebuffer[ ofs ] = word;
	}
	
	//LEAVE('X', Length);
	return Length;
}

/**
 * \fn int VGA_IOCtl(tVFS_Node *Node, int Id, void *Data)
 * \brief IO Control Call
 */
int VGA_IOCtl(tVFS_Node *Node, int Id, void *Data)
{
	switch(Id)
	{
	case DRV_IOCTL_TYPE:	return DRV_TYPE_VIDEO;
	case DRV_IOCTL_IDENT:	memcpy(Data, "VGA\0", 4);	return 1;
	case DRV_IOCTL_VERSION:	*(int*)Data = 50;	return 1;
	case DRV_IOCTL_LOOKUP:	return 0;
	
	case VIDEO_IOCTL_GETSETMODE:	return 0;	// Mode 0 only
	case VIDEO_IOCTL_FINDMODE:	return 0;	// Text Only!
	case VIDEO_IOCTL_MODEINFO:
		if( ((tVideo_IOCtl_Mode*)Data)->id != 0)	return 0;
		((tVideo_IOCtl_Mode*)Data)->width = VGA_WIDTH;
		((tVideo_IOCtl_Mode*)Data)->height = VGA_HEIGHT;
		((tVideo_IOCtl_Mode*)Data)->bpp = 4;
		return 1;
	
	case VIDEO_IOCTL_SETBUFFORMAT:
		return 0;
	
	case VIDEO_IOCTL_SETCURSOR:
		VGA_int_SetCursor( ((tVideo_IOCtl_Pos*)Data)->x, ((tVideo_IOCtl_Pos*)Data)->y );
		return 1;
	}
	return 0;
}

/**
 * \fn Uint8 VGA_int_GetColourNibble(Uint16 col)
 * \brief Converts a 12-bit colour into a VGA 4-bit colour
 */
Uint8 VGA_int_GetColourNibble(Uint16 col)
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
 * \fn Uint16 VGA_int_GetWord(tVT_Char *Char)
 * \brief Convers a character structure to a VGA character word
 */
Uint16 VGA_int_GetWord(tVT_Char *Char)
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
	
	col = VGA_int_GetColourNibble(Char->BGCol);
	ret |= col << 12;
	
	col = VGA_int_GetColourNibble(Char->FGCol);
	ret |= col << 8;
	
	return ret;
}

/**
 * \fn void VGA_int_SetCursor(Sint16 x, Sint16 y)
 * \brief Updates the cursor position
 */
void VGA_int_SetCursor(Sint16 x, Sint16 y)
{
	 int	pos = x+y*VGA_WIDTH;
	if(x == -1 || y == -1)
		pos = -1;
    outb(0x3D4, 14);
    outb(0x3D5, pos >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, pos);
}
