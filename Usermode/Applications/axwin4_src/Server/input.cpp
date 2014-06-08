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
#include <acess/devices/joystick.h>
#include <cerrno>
#include <system_error>

namespace AxWin {

CInput::CInput(const ::AxWin::CConfigInput& config, CCompositor& compositor):
	m_compositor(compositor),
	m_keyboardFD(0),
	m_mouseFD(-1)
{
	m_mouseFD = _SysOpen(config.mouse_device.c_str(), OPENFLAG_READ|OPENFLAG_WRITE);
	if( m_mouseFD == -1 )
		throw ::std::system_error(errno, ::std::system_category());
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
		const int c_n_axies = 4;
		const int c_n_buttons = 5;
		struct mouse_axis	*axies;
		uint8_t	*buttons;

		char data[sizeof(struct mouse_header) + sizeof(*axies)*c_n_axies + c_n_buttons];
		struct mouse_header	*mouseinfo = (struct mouse_header*)data;

		_SysSeek(m_mouseFD, 0, SEEK_SET);
		int len = _SysRead(m_mouseFD, data, sizeof(data));
		if( len < 0 )
			throw ::std::system_error(errno, ::std::system_category());
		
		len -= sizeof(*mouseinfo);
		if( len < 0 ) {
			_SysDebug("Mouse data undersized (%i bytes short on header)", len);
			return ;
		}
		if( mouseinfo->NAxies > c_n_axies || mouseinfo->NButtons > c_n_buttons ) {
			_SysDebug("%i axies, %i buttons above prealloc counts (%i, %i)",
				mouseinfo->NAxies, mouseinfo->NButtons, c_n_axies, c_n_buttons
				);
			return ;
		}
		if( len < sizeof(*axies)*mouseinfo->NAxies + mouseinfo->NButtons ) {
			_SysDebug("Mouse data undersized (body doesn't fit %i < %i)",
				len, sizeof(*axies)*mouseinfo->NAxies + mouseinfo->NButtons
				);
			return ;
		}

		// What? No X/Y?
		if( mouseinfo->NAxies < 2 ) {
			_SysDebug("Mouse data lacks X/Y");
			return ;
		}
	
		axies = (struct mouse_axis*)( mouseinfo + 1 );
		buttons = (uint8_t*)( axies + mouseinfo->NAxies );

		m_compositor.MouseMove(0,
			m_mouseX, m_mouseY,
			axies[0].CursorPos - m_mouseX, axies[1].CursorPos - m_mouseY
			);
		m_mouseX = axies[0].CursorPos;
		m_mouseY = axies[1].CursorPos;

		for( int i = 0; i < mouseinfo->NButtons; i ++ )
		{
			 int	bit = 1 << i;
			 int	cur = buttons[i] > 128;
			if( !!(m_mouseBtns & bit) != cur )
			{
				m_compositor.MouseButton(0, m_mouseX, m_mouseY, (eMouseButton)i, cur);
				// Flip button state
				m_mouseBtns ^= bit;
			}
		}
	}
}

};	// namespace AxWin

