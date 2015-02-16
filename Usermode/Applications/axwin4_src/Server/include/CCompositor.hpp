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


enum eMouseButton
{
	MOUSEBTN_MAIN,	// Left
	MOUSEBTN_SECONDARY,	// Right
	MOUSEBTN_MIDDLE,	// Scroll wheel
	MOUSEBTN_BTN4,
	MOUSEBTN_BTN5,
};

class CWindowIDBuffer
{
	struct TWindowID
	{
		uint16_t	Client;
		uint16_t	Window;
	};
	unsigned int	m_w;
	::std::vector<TWindowID>	m_buf;
public:
	CWindowIDBuffer(unsigned int W, unsigned int H);
	
	void set(unsigned int X, unsigned int Y, unsigned int W, unsigned int H, CWindow* win);
	CWindow* get(unsigned int X, unsigned int Y);
};

class CCompositor
{
	CVideo&	m_video;
	::std::list<CRect>	m_damageRects;
	::std::list<CWindow*>	m_windows;
	CWindow*	m_focussed_window;

	CWindowIDBuffer	m_windowIDBuffer;	// One 32-bit value per pixel
	
public:
	CCompositor(CVideo& video);

	CWindow* CreateWindow(CClient& client, const ::std::string& name);

	void	ShowWindow(CWindow* window);
	void	HideWindow(CWindow* window);

	bool	GetScreenDims(unsigned int ScrenID, unsigned int *Width, unsigned int *Height);

	void	Redraw();
	void	DamageArea(const CRect& rect);
	void	BlitFromSurface(const CSurface& dest, const CRect& src_rect);
	
	void	MouseMove(unsigned int Cursor, unsigned int X, unsigned int Y, int dX, int dY);
	void	MouseButton(unsigned int Cursor, unsigned int X, unsigned int Y, eMouseButton Button, bool Press);
	
	void	KeyState(unsigned int KeyboardID, uint32_t KeySym, bool Press, uint32_t Codepoint);
public:
	CWindow*	getWindowForCoord(unsigned int X, unsigned int Y);
};


};

#endif

