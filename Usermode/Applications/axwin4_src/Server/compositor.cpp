/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * compositor.cpp
 * - Window compositor
 */
#include <video.hpp>
#include <compositor.hpp>
#include <CCompositor.hpp>

namespace AxWin {

CCompositor::CCompositor()
{
	// 
}

CWindow* CCompositor::CreateWindow(CClient& client)
{
	return new CWindow(client, "TODO");
}

void CCompositor::Redraw()
{
	// Redraw the screen and clear damage rects
	if( m_damageRects.empty() )
		return ;
	
	// Build up foreground grid (Rects and windows)
	// - This should already be built (mutated on window move/resize/reorder)
	
	// For all windows, check for intersection with damage rects
	for( auto rect : m_damageRects )
	{
		for( auto window : m_windows )
		{
			if( rect.Contains( window->m_rect ) )
			{
				window->Repaint( rect );
			}
		}
	}

	m_damageRects.clear();
}

void CCompositor::DamageArea(const CRect& area)
{
	// 1. Locate intersection with any existing damaged areas
	// 2. Append after removing intersections
}

}	// namespace AxWin

