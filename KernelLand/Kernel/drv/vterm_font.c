/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/vterm_font.c
 * - Virtual Terminal - Font rendering code
 */
#include "vterm.h"
#include <api_drv_terminal.h>

// ---
// Font Render
// ---
#define MONOSPACE_FONT	10816

#if MONOSPACE_FONT == 10808	// 8x8
# include "vterm_font_8x8.h"
#elif MONOSPACE_FONT == 10816	// 8x16
# include "vterm_font_8x16.h"
#endif

// === PROTOTYPES ===
Uint8	*VT_Font_GetChar(Uint32 Codepoint);

// === GLOBALS ===
int	giVT_CharWidth = FONT_WIDTH;
int	giVT_CharHeight = FONT_HEIGHT;

// === CODE ===
/**
 * \brief Render a font character
 */
void VT_Font_Render(Uint32 Codepoint, void *Buffer, int Depth, int Pitch, Uint32 BGC, Uint32 FGC)
{
	Uint8	*font;
	 int	x, y;
	
	// 8-bpp and below
	if( Depth <= 8 )
	{
		Uint8	*buf = Buffer;
		
		font = VT_Font_GetChar(Codepoint);
		
		for(y = 0; y < FONT_HEIGHT; y ++)
		{
			for(x = 0; x < FONT_WIDTH; x ++)
			{
				if(*font & (1 << (FONT_WIDTH-x-1)))
					buf[x] = FGC;
				else
					buf[x] = BGC;
			}
			buf = (void*)( (tVAddr)buf + Pitch );
			font ++;
		}
	}
	// 16-bpp and below
	else if( Depth <= 16 )
	{
		Uint16	*buf = Buffer;
		
		font = VT_Font_GetChar(Codepoint);
		
		for(y = 0; y < FONT_HEIGHT; y ++)
		{
			for(x = 0; x < FONT_WIDTH; x ++)
			{
				if(*font & (1 << (FONT_WIDTH-x-1)))
					buf[x] = FGC;
				else
					buf[x] = BGC;
			}
			buf = (void*)( (tVAddr)buf + Pitch );
			font ++;
		}
	}
	// 24-bpp colour
	// - Special handling to not overwrite the next pixel
	//TODO: Endian issues here
	else if( Depth == 24 )
	{
		Uint8	*buf = Buffer;
		Uint8	bg_r = (BGC >> 16) & 0xFF;
		Uint8	bg_g = (BGC >>  8) & 0xFF;
		Uint8	bg_b = (BGC >>  0) & 0xFF;
		Uint8	fg_r = (FGC >> 16) & 0xFF;
		Uint8	fg_g = (FGC >>  8) & 0xFF;
		Uint8	fg_b = (FGC >>  0) & 0xFF;
		
		font = VT_Font_GetChar(Codepoint);
		
		for(y = 0; y < FONT_HEIGHT; y ++)
		{
			for(x = 0; x < FONT_WIDTH; x ++)
			{
				Uint8	r, g, b;
				
				if(*font & (1 << (FONT_WIDTH-x-1))) {
					r = fg_r;	g = fg_g;	b = fg_b;
				}
				else {
					r = bg_r;	g = bg_g;	b = bg_b;
				}
				buf[x*3+0] = b;
				buf[x*3+1] = g;
				buf[x*3+2] = r;
			}
			buf = (void*)( (tVAddr)buf + Pitch );
			font ++;
		}
	}
	// 32-bpp colour (nice and easy)
	else if( Depth == 32 )
	{
		Uint32	*buf = Buffer;
		
		font = VT_Font_GetChar(Codepoint);
		
		for(y = 0; y < FONT_HEIGHT; y ++)
		{
			for(x = 0; x < FONT_WIDTH; x ++)
			{
				if(*font & (1 << (FONT_WIDTH-x-1)))
					buf[x] = FGC;
				else
					buf[x] = BGC;
			}
			buf = (Uint32*)( (tVAddr)buf + Pitch );
			font ++;
		}
	}
}

/**
 * \fn Uint32 VT_Colour12to24(Uint16 Col12)
 * \brief Converts a 12-bit colour into 24 bits
 */
Uint32 VT_Colour12to24(Uint16 Col12)
{
	Uint32	ret;
	 int	tmp;
	tmp = Col12 & 0xF;
	ret  = (tmp << 0) | (tmp << 4);
	tmp = (Col12 & 0xF0) >> 4;
	ret |= (tmp << 8) | (tmp << 12);
	tmp = (Col12 & 0xF00) >> 8;
	ret |= (tmp << 16) | (tmp << 20);
	return ret;
}
/**
 * \brief Converts a 12-bit colour into 15 bits
 */
Uint16 VT_Colour12to15(Uint16 Col12)
{
	Uint32	ret;
	 int	tmp;
	tmp = Col12 & 0xF;
	ret  = (tmp << 1) | (tmp & 1);
	tmp = (Col12 & 0xF0) >> 4;
	ret |= ( (tmp << 1) | (tmp & 1) ) << 5;
	tmp = (Col12 & 0xF00) >> 8;
	ret |= ( (tmp << 1) | (tmp & 1) ) << 10;
	return ret;
}

/**
 * \brief Converts a 12-bit colour into any other depth
 * \param Col12	12-bit source colour
 * \param Depth	Desired bit depth
 * \note Green then blue get the extra avaliable bits (16:5-6-5, 14:4-5-5)
 */
Uint32 VT_Colour12toN(Uint16 Col12, int Depth)
{
	Uint32	ret;
	Uint32	r, g, b;
	 int	rSize, gSize, bSize;
	
	// Fast returns
	if( Depth == 24 )	return VT_Colour12to24(Col12);
	if( Depth == 15 )	return VT_Colour12to15(Col12);
	// - 32 is a special case, it's usually 24-bit colour with an unused byte
	if( Depth == 32 )	return VT_Colour12to24(Col12);
	
	// Bounds checks
	if( Depth < 8 )	return 0;
	if( Depth > 32 )	return 0;
	
	r = Col12 & 0xF;
	g = (Col12 & 0xF0) >> 4;
	b = (Col12 & 0xF00) >> 8;
	
	rSize = gSize = bSize = Depth / 3;
	if( rSize + gSize + bSize < Depth )	// Depth % 3 == 1
		gSize ++;
	if( rSize + gSize + bSize < Depth )	// Depth % 3 == 2
		bSize ++;
	
	// Expand
	r <<= rSize - 4;	g <<= gSize - 4;	b <<= bSize - 4;
	// Fill with the lowest bit
	if( Col12 & 0x001 )	r |= (1 << (rSize - 4)) - 1;
	if( Col12 & 0x010 )	r |= (1 << (gSize - 4)) - 1;
	if( Col12 & 0x100 )	r |= (1 << (bSize - 4)) - 1;
	
	// Create output
	ret  = r;
	ret |= g << rSize;
	ret |= b << (rSize + gSize);
	
	return ret;
}

/**
 * \fn Uint8 *VT_Font_GetChar(Uint32 Codepoint)
 * \brief Gets an index into the font array given a Unicode Codepoint
 * \note See http://en.wikipedia.org/wiki/CP437
 */
Uint8 *VT_Font_GetChar(Uint32 Codepoint)
{
	 int	index = 0;
	if(Codepoint < 128)
		return &VTermFont[Codepoint*FONT_HEIGHT];
	switch(Codepoint)
	{
	case 0xC7:	index = 128;	break;	// Ç
	case 0xFC:	index = 129;	break;	// ü
	case 0xE9:	index = 130;	break;	// é
	case 0xE2:	index = 131;	break;	// â
	case 0xE4:	index = 132;	break;	// ä
	case 0xE0:	index = 133;	break;	// à
	case 0xE5:	index = 134;	break;	// å
	case 0xE7:	index = 135;	break;	// ç
	case 0xEA:	index = 136;	break;	// ê
	case 0xEB:	index = 137;	break;	// ë
	case 0xE8:	index = 138;	break;	// è
	case 0xEF:	index = 139;	break;	// ï
	case 0xEE:	index = 140;	break;	// î
	case 0xEC:	index = 141;	break;	// ì
	case 0xC4:	index = 142;	break;	// Ä
	case 0xC5:	index = 143;	break;	// Å
	}
	
	return &VTermFont[index*FONT_HEIGHT];
}

EXPORTAS(&giVT_CharWidth, giVT_CharWidth);
EXPORTAS(&giVT_CharHeight, giVT_CharHeight);
EXPORT(VT_Font_Render);
EXPORT(VT_Colour12to24);
