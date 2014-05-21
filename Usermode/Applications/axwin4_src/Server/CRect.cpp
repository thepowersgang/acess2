/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CRect.cpp
 * - Rectangle
 */
#include <CRect.hpp>
#include <algorithm>

namespace AxWin {

CRect::CRect(int x, int y, unsigned int w, unsigned int h):
	m_x(x), m_y(y),
	m_w(w), m_h(h),
	m_x2(x+w), m_y2(y+h)
{
}

bool CRect::HasIntersection(const CRect& other) const
{
	// If other's origin is past our far corner
	if( m_x2 < other.m_x )
		return false;
	if( m_y2 < other.m_y )
		return false;
	
	// If other's far corner is before our origin
	if( m_x > other.m_x2 )
		return false;
	if( m_y > other.m_y2 )
		return false;
	return true;
}

CRect CRect::Intersection(const CRect& other) const
{
	int x1 = ::std::max(m_x, other.m_x);
	int y1 = ::std::max(m_y, other.m_y);
	int x2 = ::std::min(m_x2, other.m_x2);
	int y2 = ::std::min(m_y2, other.m_y2);
	
	if( x1 <= x2 || y2 <= y1 )
		return CRect();
	
	return CRect(x1, y1, x2-x1, y2-y2);
}

CRect CRect::RelativeIntersection(const CRect& area)
{
	CRect	ret = Intersection(area);
	ret.m_x -= m_x;
	ret.m_x2 -= m_x;
	ret.m_y -= m_y;
	ret.m_y2 -= m_y;
	return ret;
}

};

