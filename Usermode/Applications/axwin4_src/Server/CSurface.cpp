/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CWindow.cpp
 * - Window
 */
#include <CSurface.hpp>
#include <cassert>

namespace AxWin {

CSurface::CSurface(int x, int y, unsigned int w, unsigned int h):
	m_rect(x,y, w,h)
{
}

CSurface::~CSurface()
{
}

void CSurface::Resize(unsigned int W, unsigned int H)
{
	assert(!"TODO: CSurface::Resize");
}

const uint32_t* CSurface::GetScanline(unsigned int row, unsigned int x_ofs) const
{
	return 0;
}


};	// namespace AxWin


