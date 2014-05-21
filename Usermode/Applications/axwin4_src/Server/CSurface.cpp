/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CWindow.cpp
 * - Window
 */
#include <CSurface.hpp>

namespace AxWin {

CSurface::CSurface(unsigned int x, unsigned int y, unsigned int w, unsigned int h):
	m_rect(x,y, w,h)
{
}

CSurface::~CSurface()
{
}

const uint32_t* CSurface::GetScanline(unsigned int row, unsigned int x_ofs) const
{
	return 0;
}


};	// namespace AxWin


