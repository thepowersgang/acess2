/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * CCompositor.hpp
 * - Window Compositor
 */
#ifndef _CCOMPOSITOR_H_
#define _CCOMPOSITOR_H_

#include <string>
#include <list>
#include <vector>
#include "CRect.hpp"
#include "CWindow.hpp"

namespace AxWin {

class CClient;
class CVideo;

struct TWindowID
{
	uint16_t	Client;
	uint16_t	Window;
};

enum eMouseButton
{
	MOUSEBTN_MAIN,	// Left
	MOUSEBTN_SECONDARY,	// Right
	MOUSEBTN_MIDDLE,	// Scroll wheel
	MOUSEBTN_BTN4,
	MOUSEBTN_BTN5,
};

class CCompositor
{
	CVideo&	m_video;
	::std::list<CRect>	m_damageRects;
	::std::list<CWindow*>	m_windows;

	::std::vector<TWindowID>	m_windowIDBuffer;	// One 32-bit value per pixel
	//::std::vector<CPixel> 	m_frameBuffer;	// Local copy of the framebuffer (needed?)
	
public:
	CCompositor(CVideo& video);

	CWindow* CreateWindow(CClient& client, const ::std::string& name);

	bool	GetScreenDims(unsigned int ScrenID, unsigned int *Width, unsigned int *Height);

	void	Redraw();
	void	DamageArea(const CRect& rect);
	void	BlitFromSurface(const CSurface& dest, const CRect& src_rect);
	
	void	MouseMove(unsigned int Cursor, unsigned int X, unsigned int Y, int dX, int dY);
	void	MouseButton(unsigned int Cursor, unsigned int X, unsigned int Y, eMouseButton Button, bool Press);
	
	void	KeyState(unsigned int KeyboardID, uint32_t KeySym, bool Press, uint32_t Codepoint);
};


};

#endif

