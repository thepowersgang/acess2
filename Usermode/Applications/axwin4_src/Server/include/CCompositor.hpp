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
#include "CRect.hpp"
#include "CWindow.hpp"

namespace AxWin {

class CClient;

class CCompositor
{
	::std::list<CRect>	m_damageRects;
	::std::list<CWindow*>	m_windows;

public:
	CCompositor();

	CWindow* CreateWindow(CClient& client);

	void	Redraw();
	void	DamageArea(const CRect& rect);
};


};

#endif

