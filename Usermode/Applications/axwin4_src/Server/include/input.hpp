/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * input.hpp
 * - Input Interface Header
 */
#ifndef _INPUT_H_
#define _INPUT_H_

#include <acess/sys.h>

namespace AxWin {

class CCompositor;

class CInput
{
	CCompositor&	m_compositor;
	 int	m_keyboardFD;
	 int	m_mouseFD;
	
	unsigned int m_mouseX;
	unsigned int m_mouseY;
	unsigned int m_mouseBtns;
public:
	CInput(const CConfigInput& config, CCompositor& compositor);
	 int FillSelect(::fd_set& rfds);
	void HandleSelect(::fd_set& rfds);
};

};	// namespace AxWin

#endif

