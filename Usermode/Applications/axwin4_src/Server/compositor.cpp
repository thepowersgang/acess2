/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * compositor.cpp
 * - Window compositor
 */
#include <video.hpp>
#include <CCompositor.hpp>
#include <cassert>

namespace AxWin {

CCompositor::CCompositor(CVideo& video):
	m_video(video)
{
	// 
}

CWindow* CCompositor::CreateWindow(CClient& client, const ::std::string& name)
{
	return new CWindow(*this, client, name);
}

bool CCompositor::GetScreenDims(unsigned int ScreenID, unsigned int* W, unsigned int* H)
{
	assert(W && H);
	if( ScreenID != 0 )
	{
		*W = 0;
		*H = 0;
		return false;
	}
	else
	{
		m_video.GetDims(*W, *H);
		return true;
	}
}

void CCompositor::Redraw()
{
	_SysDebug("CCompositor::Redraw");
	// Redraw the screen and clear damage rects
	if( m_damageRects.empty() )
		return ;
	
	// Build up foreground grid (Rects and windows)
	// - This should already be built (mutated on window move/resize/reorder)
	
	// For all windows, check for intersection with damage rects
	for( auto rect : m_damageRects )
	{
		_SysDebug("rect=(%i,%i) %ix%i", rect.m_x, rect.m_y, rect.m_w, rect.m_h);
		// window list should be sorted by draw order (lowest first)
		for( auto window : m_windows )
		{
			if( rect.HasIntersection( window->m_surface.m_rect ) )
			{
				// TODO: just reblit
				CRect	rel_rect = window->m_surface.m_rect.RelativeIntersection(rect);
				BlitFromSurface( window->m_surface, rel_rect );
				//window->Repaint( rel_rect );
			}
		}
		
		// TODO: Blit from windows to a local surface, then blit from there to screen here
	}

	m_damageRects.clear();
	m_video.Flush();
}

void CCompositor::DamageArea(const CRect& area)
{
	m_damageRects.push_back( area );
	// 1. Locate intersection with any existing damaged areas
	// 2. Append after removing intersections
}

void CCompositor::BlitFromSurface(const CSurface& dest, const CRect& src_rect)
{
	for( unsigned int i = 0; i < src_rect.m_h; i ++ )
	{
		m_video.BlitLine(
			dest.GetScanline(src_rect.m_y, src_rect.m_y),
			dest.m_rect.m_y + src_rect.m_y + i,
			dest.m_rect.m_x + src_rect.m_x,
			src_rect.m_w
			);
	}
}

}	// namespace AxWin

