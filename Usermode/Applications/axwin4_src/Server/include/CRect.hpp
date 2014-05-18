/*
 */
#ifndef _CRECT_H_
#define _CRECT_H_

namespace AxWin {

class CRect
{
public:
	CRect(int X, int Y, int W, int H);
	
	bool Contains(const CRect& other) const;
};

};	// namespace AxWin

#endif

