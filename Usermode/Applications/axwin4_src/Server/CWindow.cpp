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
	if( m_is_shown )
	{
		CRect	outrect(
			m_surface.m_rect.m_x + rect.m_x, 
			m_surface.m_rect.m_y + rect.m_y, 
			rect.m_w, rect.m_h
			);
		m_compositor.DamageArea(outrect);
	}
}

void CWindow::Show(bool bShow)
{
	if( m_is_shown == bShow )
		return;
	
	if( bShow )
		m_compositor.ShowWindow( this );
	else
		m_compositor.HideWindow( this );
	m_is_shown = bShow;
}

void CWindow::Move(int X, int Y)
{
	m_surface.m_rect.Move(X, Y);
}
void CWindow::Resize(unsigned int W, unsigned int H)
{
	m_surface.Resize(W, H);
	IPC::SendMessage_NotifyDims(m_client, W, H);
}
void CWindow::SetFlags(uint32_t Flags)
{
	// TODO: CWindow::SetFlags
	_SysDebug("TOOD: CWindow::SetFlags");
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


void CWindow::DrawScanline(unsigned int row, unsigned int x, unsigned int w, const uint8_t *data)
{
	m_surface.DrawScanline(row, x, w, data);
	CRect	damaged( m_surface.m_rect.m_x+x, m_surface.m_rect.m_y+row, w, 1 );
	m_compositor.DamageArea(damaged);
}

};

