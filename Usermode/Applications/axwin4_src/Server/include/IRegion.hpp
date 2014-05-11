/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * IRegion.hpp
 * - Representation of a primitive region in a window (part of the window render list)
 */
#ifndef _IREGION_HPP_
#define _IREGION_HPP_

#include <string>
#include <vector>
#include "CRect.hpp"

namespace AxWin {

class IRegion
{
protected:
	CWindow&	m_parentWindow;
public:
	virtual IRegion(CWindow& parent, const ::AxWin::Rect& position);
	virtual ~IRegion();
	
	virtual void Redraw(const ::AxWin::Rect& area) = 0;
	virtual bool SetAttr(unsigned int Index, const IPCAttrib& Value) = 0;
};

};

#endif

