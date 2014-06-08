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
#include <ipc.hpp>

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
	m_surface.m_rect.Move(X, Y);
}
void CWindow::Resize(unsigned int W, unsigned int H)
{
	m_surface.Resize(W, H);
	IPC::SendNotify_Dims(m_client, W, H);
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

