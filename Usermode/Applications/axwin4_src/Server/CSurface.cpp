/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CWindow.cpp
 * - Window
 */
#include <CSurface.hpp>
#include <cassert>
#include <stdexcept>
#include <cstring>

namespace AxWin {

CSurface::CSurface(int x, int y, unsigned int w, unsigned int h):
	m_rect(x,y, w,h)
{
	if( w > 0 && h > 0 )
	{
		m_data = new uint32_t[w * h];
	}
}

CSurface::~CSurface()
{
}

void CSurface::Resize(unsigned int W, unsigned int H)
{
	// Easy realloc
	// TODO: Should I maintain window contents sanely? NOPE!
	delete m_data;
	m_data = new uint32_t[W * H];
	m_rect.Resize(W, H);
}

void CSurface::DrawScanline(unsigned int row, unsigned int x_ofs, unsigned int w, const void* data)
{
	_SysDebug("DrawScanline(%i,%i,%i,%p)", row, x_ofs, w, data);
	if( row >= m_rect.m_h )
		throw ::std::out_of_range("CSurface::DrawScanline row");
	if( x_ofs >= m_rect.m_w )
		throw ::std::out_of_range("CSurface::DrawScanline x_ofs");

	if( w > m_rect.m_w )
		throw ::std::out_of_range("CSurface::DrawScanline width");
	
	_SysDebug(" memcpy(%p, %p, %i)", &m_data[row*m_rect.m_w + x_ofs], data, w*4 );
	::memcpy( &m_data[row*m_rect.m_w + x_ofs], data, w*4 );
}

const uint32_t* CSurface::GetScanline(unsigned int row, unsigned int x_ofs) const
{
	if( row >= m_rect.m_h )
		throw ::std::out_of_range("CSurface::GetScanline row");
	if( x_ofs >= m_rect.m_w )
		throw ::std::out_of_range("CSurface::GetScanline x_ofs");

	return &m_data[row * m_rect.m_w + x_ofs];
}


};	// namespace AxWin


