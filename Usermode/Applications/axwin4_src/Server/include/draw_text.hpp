/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * draw_text.hpp
 * - Text drawing classes
 */
#ifndef _DRAW_TEXT_HPP_
#define _DRAW_TEXT_HPP_

#include "IFontFace.hpp"

namespace AxWin {

class CFontFallback:
	public IFontFace
{
public:
	CFontFallback();

	CRect Size(const ::std::string& text, unsigned int Size) const override;
	void Render(CSurface& dest, const CRect& rect, const ::std::string& text, unsigned int Size) override;
private:
	void renderAtRes(CSurface& dest, const CRect& rect, uint32_t cp, unsigned int Size, uint32_t FGC);
	static uint8_t getValueAtPt(const uint8_t* char_ptr, unsigned int xf16, unsigned int yf16);
	static uint8_t getValueAtRaw(const uint8_t* char_ptr, unsigned int x, unsigned int y);
	unsigned int unicodeToCharmap(uint32_t cp) const;
};

#if FREETYPE_ENABLED
class CFontFT:
	public IFontFace
{
	FT_Face	m_face;
public:
	CFontFT(const char *Filename);
	~CFontFT();

	CRect Size(const ::std::string& text, unsigned int Size) const override;
	void Render(CSurface& dest, const CRect& rect, const ::std::string& text, unsigned int Size) override;
};
#endif	// FREETYPE_ENABLED

}

#endif

