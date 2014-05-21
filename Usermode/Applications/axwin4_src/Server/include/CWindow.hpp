/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CWindow.hpp
 * - Window class
 */
#ifndef _CWINDOW_HPP_
#define _CWINDOW_HPP_

#include <string>
#include <vector>
#include <cstdint>
#include "CRect.hpp"
#include "CSurface.hpp"

namespace AxWin {

class CClient;

class CWindow
{
public:
	CWindow(CClient& client, const ::std::string &name);
	~CWindow();
	
	void Repaint(const CRect& rect);
	
	void MouseButton(int ButtonID, int X, int Y, bool Down);
	void MouseMove(int NewX, int NewY);
	void KeyEvent(::uint32_t Scancode, const ::std::string &Translated, bool Down);

	CSurface	m_surface;
private:
	const ::std::string	m_name;
	CClient&	m_client;
	//::std::list<CRegion*>	m_regions;
};

};	// namespace AxWin

#endif


