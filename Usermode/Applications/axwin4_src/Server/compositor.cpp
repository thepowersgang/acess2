/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * compositor.cpp
 * - Window compositor
 */
#include <CVideo.hpp>
#include <CCompositor.hpp>

namespace AxWin {

CCompositor*	CCompositor::s_instance;

void CCompositor::Initialise(const CConfigCompositor& config)
{
	assert(!CCompositor::s_instance);
	CCompositor::s_instance = new CCompositor(config);
}

CCompositor::CCompositor(const CConfigCompositor& config):
	m_config(config)
{
	// 
}

IWindow* CCompositor::CreateWindow(CClient& client)
{
	return new CWindow(client);
}

void CCompositor::Redraw()
{
	// Redraw the screen and clear damage rects
	if( m_damageRects.empty() )
		return ;
	
	// For all windows, check for intersection with damage rect
}

void CCompositor::DamageArea(const Rect& area)
{

}

}	// namespace AxWin

