/*
 */
#ifndef _CSURFACE_H_
#define _CSURFACE_H_

#include <cstdint>
#include "CRect.hpp"

namespace AxWin {

class CSurface
{
public:
	CSurface(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
	~CSurface();
	
	const uint32_t* GetScanline(unsigned int row, unsigned int x_ofs) const;
	
	CRect	m_rect;
	uint32_t*	m_data;
};

};	// namespace AxWin

#endif

