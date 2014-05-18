/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * IWindow.hpp
 * - Window abstract base class
 */
#ifndef _IWINDOW_HPP_
#define _IWINDOW_HPP_

#include <string>
#include <vector>
#include "CRect.hpp"

namespace AxWin {

class IWindow
{
public:
	virtual IWindow(const ::std::string &name);
	virtual ~IWindow();
	
	virtual void Repaint() = 0;
	
	virtual void MouseButton(int ButtonID, int X, int Y, bool Down);
	virtual void MouseMove(int NewX, int NewY);
	virtual void KeyEvent(uint32_t Scancode, const ::std::string &Translated, bool Down);
protected:
	const ::std::string	m_name;
};

}	// namespace AxWin

#endif

