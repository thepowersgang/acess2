/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CWindow.cpp
 * - Window
 */
#include <CWindow.hpp>
#include <CCompositor.hpp>
#include <assert.h>

namespace AxWin {

CWindow::CWindow(CCompositor& compositor, CClient& client, const ::std::string& name):
	m_surface(0,0,0,0),
	m_compositor(compositor),
	m_client(client),
	m_name(name)
{
	_SysDebug("CWindow::CWindow()");
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
			rgn->Repaint(m_surface, rel_rect);
		} 
	}
	#endif
}

void CWindow::Show(bool bShow)
{
	assert(!"TODO: CWindow::Show");
}

void CWindow::Move(int X, int Y)
{
	assert(!"TODO: CWindow::Move");
}
void CWindow::Resize(unsigned int W, unsigned int H)
{
	assert(!"TODO: CWindow::Resize");
}
uint64_t CWindow::ShareSurface()
{
	assert(!"TODO: CWindow::ShareSurface");
	return 0;
}

void CWindow::MouseButton(int ButtonID, int X, int Y, bool Down)
{
}

void CWindow::MouseMove(int NewX, int NewY)
{
}

void CWindow::KeyEvent(::uint32_t Scancode, const ::std::string &Translated, bool Down)
{
}

};

