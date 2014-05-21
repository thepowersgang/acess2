/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CWindow.cpp
 * - Window
 */
#include <CWindow.hpp>

namespace AxWin {

CWindow::CWindow(CClient& client, const ::std::string& name):
	m_client(client),
	m_name(name),
	m_surface(0,0,0,0)
{
	
}

CWindow::~CWindow()
{
}

void CWindow::Repaint(const CRect& rect)
{
	#if 0
	for( auto rgn : m_regions )
	{
		if( rect.Contains(rgn->m_rect) )
		{
			CRect	rel_rect(rect, rgn->m_rect);
			rgn->Repaint();
		} 
	}
	#endif
}

};

