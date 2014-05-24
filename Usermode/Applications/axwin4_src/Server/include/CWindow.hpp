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
class CCompositor;
class CRegion;

class CWindow
{
public:
	CWindow(CCompositor& compositor, CClient& client, const ::std::string &name);
	~CWindow();
	
	void Repaint(const CRect& rect);

	void Show(bool bShow);	
	void Move(int X, int Y);
	void Resize(unsigned int W, unsigned int H);
	
	uint64_t ShareSurface();
	
	void MouseButton(int ButtonID, int X, int Y, bool Down);
	void MouseMove(int NewX, int NewY);
	void KeyEvent(::uint32_t Scancode, const ::std::string &Translated, bool Down);

	CSurface	m_surface;
private:
	CCompositor&	m_compositor;
	CClient&	m_client;
	const ::std::string	m_name;
	::std::vector<CRegion*>	m_regions;
};

};	// namespace AxWin

#endif


