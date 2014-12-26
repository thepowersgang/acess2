/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * draw_text.cpp
 * - Text Drawing
 *
 * Handles font selection and drawing of text to windows
 */
#include <draw_text.hpp>
#include <axwin4/definitions.h>
#include <unicode.h>	// libunicode (acess)
extern "C" {
#include <assert.h>	// assert... and _SysDebug
};
#include "resources/font_8x16.h"

// === CODE ===
namespace AxWin {

// -- Primitive fallback font
CFontFallback::CFontFallback()
{
}

CRect CFontFallback::Size(const ::std::string& text, unsigned int Size) const
{
	return CRect(0,0, text.size() * Size * FONT_WIDTH / FONT_HEIGHT, Size);
}

/**
 * \param Text height in pixels (not in points)
 */
void CFontFallback::Render(CSurface& dest, const CRect& rect, const ::std::string& text, unsigned int Size)
{
	unsigned int font_step = Size * FONT_WIDTH / FONT_HEIGHT;
	CRect	pos = rect;
	for( auto codepoint : ::libunicode::utf8string(text) )
	{
		renderAtRes(dest, pos, codepoint, Size, 0x000000);
		pos.Translate(font_step, 0);
	}
}

void CFontFallback::renderAtRes(CSurface& dest, const CRect& rect, uint32_t cp, unsigned int Size, uint32_t FGC)
{
	unsigned int char_idx = unicodeToCharmap(cp);
	assert(char_idx < 256);
	const uint8_t*	char_ptr = &VTermFont[char_idx * FONT_HEIGHT];
	unsigned int	out_h = Size;
	unsigned int	out_w = (Size * FONT_WIDTH / FONT_HEIGHT);
	uint32_t	char_data[out_w];
	if( Size == FONT_HEIGHT ) {
		// Standard blit
		for( unsigned int row = 0; row < out_h; row ++ )
		{
			for( unsigned int col = 0; col < out_w; col ++ )
			{
				uint8_t alpha = getValueAtRaw(char_ptr, col, row);
				char_data[col] = ((uint32_t)alpha << 24) | (FGC & 0xFFFFFF);
			}
			dest.BlendScanline(rect.m_y + row, rect.m_x, FONT_WIDTH, char_data);
		}
	}
	else if( Size < FONT_HEIGHT ) {
		// Down-scaled blit
	}
	else {
		// up-scaled blit
		for( unsigned int row = 0; row < out_h; row ++ )
		{
			unsigned int yf16 = row * FONT_HEIGHT * 0x10000 / out_h;
			for( unsigned int col = 0; col < out_w; col ++ )
			{
				unsigned int xf16 = col * FONT_WIDTH * 0x10000 / out_w;
				uint8_t alpha = getValueAtPt(char_ptr, xf16, yf16);
				//_SysDebug("row %i (%05x), col %i (%05x): alpha = %02x", row, yf16, col, xf16, alpha);
				char_data[col] = ((uint32_t)alpha << 24) | (FGC & 0xFFFFFF);
			}
			dest.BlendScanline(rect.m_y + row, rect.m_x, out_w, char_data);
		}
	}
}
// X and Y are fixed-point 16.16 values
uint8_t CFontFallback::getValueAtPt(const uint8_t* char_ptr, unsigned int xf16, unsigned int yf16)
{
	unsigned int ix = xf16 >> 16;
	unsigned int iy = yf16 >> 16;
	unsigned int fx = xf16 & 0xFFFF;
	unsigned int fy = yf16 & 0xFFFF;
	
	if( fx == 0 && fy == 0 ) {
		return getValueAtRaw(char_ptr, ix, iy);
	}
	else if( fx == 0 ) {
		float y = (float)fy / 0x10000;
		uint8_t v0 = getValueAtRaw(char_ptr, ix, iy  );
		uint8_t v1 = getValueAtRaw(char_ptr, ix, iy+1);
		return v0 * (1 - y) + v1 * y;
	}
	else if( fy == 0 ) {
		float x = (float)fx / 0x10000;
		uint8_t v0 = getValueAtRaw(char_ptr, ix  , iy);
		uint8_t v1 = getValueAtRaw(char_ptr, ix+1, iy);
		return v0 * (1 - x) + v1 * x;
	}
	else {
		float x = (float)fx / 0x10000;
		float y = (float)fx / 0x10000;
		// [0,0](1 - x)(1 - y) + [1,0]x(1-y) + [0,1](1-x)y + [1,1]xy
		uint8_t v00 = getValueAtRaw(char_ptr, ix, iy);
		uint8_t v01 = getValueAtRaw(char_ptr, ix, iy+1);
		uint8_t v10 = getValueAtRaw(char_ptr, ix+1, iy);
		uint8_t v11 = getValueAtRaw(char_ptr, ix+1, iy+1);
		//_SysDebug("x,y = %04x %04x", (unsigned)(x * 0x10000), (unsigned)(y * 0x10000));
		//_SysDebug("v = %02x %02x %02x %02x", v00, v01, v10, v11);
		float val1 = v00 * (1 - x) * (1 - y);
		float val2 = v10 * x * (1 - y);
		float val3 = v01 * (1 - x) * y;
		float val4 = v11 * x * y;
		//_SysDebug("vals = %04x %04x %04x %04x",
		//	(unsigned)(val1 * 0x10000),
		//	(unsigned)(val2 * 0x10000),
		//	(unsigned)(val3 * 0x10000),
		//	(unsigned)(val4 * 0x10000)
		//	);
		
		return (uint8_t)(val1 + val2 + val3 + val4);
	}
}

uint8_t CFontFallback::getValueAtRaw(const uint8_t* char_ptr, unsigned int x, unsigned int y)
{
	//if( x == 0 || y == 0 )
	//	return 0;
	//x --; y --;
	if(x >= FONT_WIDTH || y >= FONT_HEIGHT)
		return 0;
	return (char_ptr[y] & (1 << (7-x))) ? 255 : 0;
}

unsigned int CFontFallback::unicodeToCharmap(uint32_t cp) const
{
	if(cp >= ' ' && cp < 0x7F)
		return cp;
	switch(cp)
	{
	default:
		return 0;
	}
}

};	// namespace AxWin

