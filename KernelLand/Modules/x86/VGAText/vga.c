/*
 * Acess2 VGA Controller Driver
 */
#define DEBUG	0
#include <acess.h>
#include <fs_devfs.h>
#include <api_drv_video.h>
#include <modules.h>

// === CONSTANTS ===
#define	VGA_WIDTH	80
#define	VGA_HEIGHT	25

// === PROTOTYPES ===
 int	VGA_Install(char **Arguments);
size_t	VGA_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer);
 int	VGA_IOCtl(tVFS_Node *Node, int Id, void *Data);
Uint8	VGA_int_GetColourNibble(Uint16 col);
Uint16	VGA_int_GetWord(const tVT_Char *Char);
void	VGA_int_SetCursor(Sint16 x, Sint16 y);
// --- 2D Acceleration Functions --
void	VGA_2D_Fill(void *Ent, Uint16 X, Uint16 Y, Uint16 W, Uint16 H, Uint32 Colour);
void	VGA_2D_Blit(void *Ent, Uint16 DstX, Uint16 DstY, Uint16 SrcX, Uint16 SrcY, Uint16 W, Uint16 H);

// === GLOBALS ===
MODULE_DEFINE(0, 0x000A, x86_VGAText, VGA_Install, NULL, NULL);
tVFS_NodeType	gVGA_NodeType = {
	//.Read = VGA_Read,
	.Write = VGA_Write,
	.IOCtl = VGA_IOCtl
	};
tDevFS_Driver	gVGA_DevInfo = {
	NULL, "x86_VGAText",
	{
	.NumACLs = 0,
	.Size = VGA_WIDTH*VGA_HEIGHT*sizeof(tVT_Char),
	.Type = &gVGA_NodeType
	}
};
Uint16	*gVGA_Framebuffer = (void*)( KERNEL_BASE|0xB8000 );
 int	giVGA_BufferFormat = VIDEO_BUFFMT_TEXT;
tDrvUtil_Video_2DHandlers	gVGA_2DFunctions = {
	NULL,
	VGA_2D_Fill,
	VGA_2D_Blit
};

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
 * \brief Writes a string of bytes to the VGA controller
 */
size_t VGA_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer)
{
	if( giVGA_BufferFormat == VIDEO_BUFFMT_TEXT )
	{
		 int	num = Length / sizeof(tVT_Char);
		 int	ofs = Offset / sizeof(tVT_Char);
		 int	i = 0;
		const tVT_Char	*chars = Buffer;
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
	else if( giVGA_BufferFormat == VIDEO_BUFFMT_2DSTREAM )
	{
		return DrvUtil_Video_2DStream(NULL, Buffer, Length, &gVGA_2DFunctions, sizeof(gVGA_2DFunctions));
	}
	else
	{
		return 0;
	}
}

/**
 * \fn int VGA_IOCtl(tVFS_Node *Node, int Id, void *Data)
 * \brief IO Control Call
 */
int VGA_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	 int	rv;
	switch(ID)
	{
	case DRV_IOCTL_TYPE:	return DRV_TYPE_VIDEO;
	case DRV_IOCTL_IDENT:	memcpy(Data, "VGA\0", 4);	return 1;
	case DRV_IOCTL_VERSION:	*(int*)Data = 50;	return 1;
	case DRV_IOCTL_LOOKUP:	return 0;
	
	case VIDEO_IOCTL_GETSETMODE:	return 0;	// Mode 0 only
	case VIDEO_IOCTL_FINDMODE:
		if( !CheckMem(Data, sizeof(tVideo_IOCtl_Mode)) )	return -1;
		((tVideo_IOCtl_Mode*)Data)->id = 0;	// Text Only!
	case VIDEO_IOCTL_MODEINFO:
		if( !CheckMem(Data, sizeof(tVideo_IOCtl_Mode)) )	return -1;
		if( ((tVideo_IOCtl_Mode*)Data)->id != 0)	return 0;
		((tVideo_IOCtl_Mode*)Data)->width = VGA_WIDTH*giVT_CharWidth;
		((tVideo_IOCtl_Mode*)Data)->height = VGA_HEIGHT*giVT_CharHeight;
		((tVideo_IOCtl_Mode*)Data)->bpp = 4;
		return 1;
	
	case VIDEO_IOCTL_SETBUFFORMAT:
		if( !CheckMem(Data, sizeof(int)) )	return -1;
		switch( *(int*)Data )
		{
		case VIDEO_BUFFMT_TEXT:
		case VIDEO_BUFFMT_2DSTREAM:
			rv = giVGA_BufferFormat;
			giVGA_BufferFormat = *(int*)Data;
//			Log_Debug("VGA", "Buffer format set to %i", giVGA_BufferFormat);
			return rv;
		default:
			break;
		}
		return -1;
	
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
Uint16 VGA_int_GetWord(const tVT_Char *Char)
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

void VGA_2D_Fill(void *Ent, Uint16 X, Uint16 Y, Uint16 W, Uint16 H, Uint32 Colour)
{
	tVT_Char	ch;
	Uint16	word, *buf;

	X /= giVT_CharWidth;
	W /= giVT_CharWidth;
	Y /= giVT_CharHeight;
	H /= giVT_CharHeight;

	ch.Ch = 0x20;
	ch.BGCol  = (Colour & 0x0F0000) >> (16-8);
	ch.BGCol |= (Colour & 0x000F00) >> (8-4);
	ch.BGCol |= (Colour & 0x00000F);
	ch.FGCol = 0;
	word = VGA_int_GetWord(&ch);

	Log("Fill (%i,%i) %ix%i with 0x%x", X, Y, W, H, word);

	if( X > VGA_WIDTH || Y > VGA_HEIGHT )	return ;
	if( X + W > VGA_WIDTH )	W = VGA_WIDTH - X;
	if( Y + H > VGA_HEIGHT )	H = VGA_HEIGHT - Y;

	buf = gVGA_Framebuffer + Y*VGA_WIDTH + X;

	
	while( H -- ) {
		 int	i;
		for( i = 0; i < W; i ++ )
			*buf++ = word;
		buf += VGA_WIDTH - W;
	}
}

void VGA_2D_Blit(void *Ent, Uint16 DstX, Uint16 DstY, Uint16 SrcX, Uint16 SrcY, Uint16 W, Uint16 H)
{
	Uint16	*src, *dst;

	DstX /= giVT_CharWidth;
	SrcX /= giVT_CharWidth;
	W /= giVT_CharWidth;

	DstY /= giVT_CharHeight;
	SrcY /= giVT_CharHeight;
	H /= giVT_CharHeight;

//	Log("(%i,%i) from (%i,%i) %ix%i", DstX, DstY, SrcX, SrcY, W, H);

	if( SrcX > VGA_WIDTH || SrcY > VGA_HEIGHT )	return ;
	if( SrcX + W > VGA_WIDTH )	W = VGA_WIDTH - SrcX;
	if( SrcY + H > VGA_HEIGHT )	H = VGA_HEIGHT - SrcY;
	if( DstX > VGA_WIDTH || DstY > VGA_HEIGHT )	return ;
	if( DstX + W > VGA_WIDTH )	W = VGA_WIDTH - DstX;
	if( DstY + H > VGA_HEIGHT )	H = VGA_HEIGHT - DstY;


	src = gVGA_Framebuffer + SrcY*VGA_WIDTH + SrcX;
	dst = gVGA_Framebuffer + DstY*VGA_WIDTH + DstX;

	if( src > dst )
	{
		// Simple copy
		while( H-- ) {
			memcpy(dst, src, W*2);
			dst += VGA_WIDTH;
			src += VGA_WIDTH;
		}
	}
	else
	{
		dst += H*VGA_WIDTH;
		src += H*VGA_WIDTH;
		while( H -- ) {
			 int	i;
			dst -= VGA_WIDTH-W;
			src -= VGA_WIDTH-W;
			for( i = W; i --; )	*--dst = *--src;
		}
	}
}
