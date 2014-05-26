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
	if( m_mouseFD != -1 )
		FD_SET(m_mouseFD, &rfds);
	return ::std::max(m_keyboardFD, m_mouseFD)+1;
}

void CInput::HandleSelect(::fd_set& rfds)
{
	if( FD_ISSET(m_keyboardFD, &rfds) )
	{
		uint32_t	codepoint;
		static uint32_t	scancode;
		#define KEY_CODEPOINT_MASK	0x3FFFFFFF
		
		size_t readlen = _SysRead(m_keyboardFD, &codepoint, sizeof(codepoint));
		if( readlen != sizeof(codepoint) )
		{
			// oops, error
			_SysDebug("Terminal read failed? (%i != %i)", readlen, sizeof(codepoint));
		}
	
//		_SysDebug("Keypress 0x%x", codepoint);
	
		switch(codepoint & 0xC0000000)
		{
		case 0x00000000:	// Key pressed
			//WM_Input_KeyDown(codepoint & KEY_CODEPOINT_MASK, scancode);
		case 0x80000000:	// Key release
			//WM_Input_KeyFire(codepoint & KEY_CODEPOINT_MASK, scancode);
			scancode = 0;
			break;
		case 0x40000000:	// Key refire
			//WM_Input_KeyUp(codepoint & KEY_CODEPOINT_MASK, scancode);
			scancode = 0;
			break;
		case 0xC0000000:	// Raw scancode
			scancode = codepoint & KEY_CODEPOINT_MASK;
			break;
		}
	}
	
	if( m_mouseFD != -1 && FD_ISSET(m_mouseFD, &rfds) )
	{
		// TODO: Read mouse event and handle
	}
}

};	// namespace AxWin

