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

// === CODE ===
namespace AxWin {

CFont::CFont():
	
{
}

CFont::CFont(const char *Path):
	
{
}

/**
 * \param Text height in pixels (not in points)
 */
void CFont::Render(CSurface& dest, const CRect& rect, const ::std::string& text, unsigned int Size) const
{

}

};	// namespace AxWin

