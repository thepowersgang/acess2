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
	m_rect(0,0,0,0)
{
	
}

CWindow::~CWindow()
{
}

void CWindow::Repaint(const CRect& rect)
{
	
}

};

