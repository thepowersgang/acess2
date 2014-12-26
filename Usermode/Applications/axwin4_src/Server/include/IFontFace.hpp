/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * IFont.hpp
 * - Text drawing (font rendering) primitive
 */
#ifndef _IFONT_HPP_
#define _IFONT_HPP_

#include <string>
#include "CRect.hpp"
#include "CSurface.hpp"

namespace AxWin {

class IFontFace
{
public:
	virtual CRect Size(const ::std::string& text, unsigned int Size) const = 0;
	virtual void Render(CSurface& dest, const CRect& rect, const ::std::string& text, unsigned int Size) = 0;
};

};

#endif

