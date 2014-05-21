/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * CCompositor.hpp
 * - Window Compositor
 */
#ifndef _CCOMPOSITOR_H_
#define _CCOMPOSITOR_H_

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

class CCompositor
{
	CVideo&	m_video;
	::std::list<CRect>	m_damageRects;
	::std::list<CWindow*>	m_windows;

	::std::vector<TWindowID>	m_windowIDBuffer;	// One 32-bit value per pixel
	//::std::vector<CPixel> 	m_frameBuffer;	// Local copy of the framebuffer (needed?)
	
public:
	CCompositor(CVideo& video);

	CWindow* CreateWindow(CClient& client);

	void	Redraw();
	void	DamageArea(const CRect& rect);
	void	BlitFromSurface(const CSurface& dest, const CRect& src_rect);
};


};

#endif

