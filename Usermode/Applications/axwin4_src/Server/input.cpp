/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * input.cpp
 * - Input
 */
#include <CConfigInput.hpp>
#include <input.hpp>
#include <CCompositor.hpp>
#include <algorithm>

namespace AxWin {

CInput::CInput(const ::AxWin::CConfigInput& config, CCompositor& compositor):
	m_compositor(compositor),
	m_keyboardFD(0),
	m_mouseFD(-1)
{
	
}

int CInput::FillSelect(::fd_set& rfds)
{
	FD_SET(m_keyboardFD, &rfds);
	FD_SET(m_mouseFD, &rfds);
	return ::std::max(m_keyboardFD, m_mouseFD)+1;
}

void CInput::HandleSelect(::fd_set& rfds)
{
	if( FD_ISSET(m_keyboardFD, &rfds) )
	{
		// TODO: Read keystroke and handle
	}
	
	if( FD_ISSET(m_mouseFD, &rfds) )
	{
		// TODO: Read mouse event and handle
	}
}

};	// namespace AxWin

