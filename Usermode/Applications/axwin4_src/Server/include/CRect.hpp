/*
 */
#ifndef _CRECT_H_
#define _CRECT_H_

namespace AxWin {

class CRect
{
public:
	CRect():
		CRect(0,0,0,0)
	{
	};
	CRect(int X, int Y, unsigned int W, unsigned int H);
	
	bool HasIntersection(const CRect& other) const;
	CRect Intersection(const CRect& other) const;
	
	CRect RelativeIntersection(const CRect& area);
	
	int	m_x;
	int	m_y;
	int	m_w;
	int	m_h;
	int	m_x2;
	int	m_y2;
};

};	// namespace AxWin

#endif

