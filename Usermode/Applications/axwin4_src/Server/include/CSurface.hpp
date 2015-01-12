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
	CSurface(int x, int y, unsigned int w, unsigned int h);
	~CSurface();

	uint64_t GetSHMHandle();
	
	void Resize(unsigned int new_w, unsigned int new_h);
	
	void DrawScanline(unsigned int row, unsigned int x_ofs, unsigned int w, const void* data);
	void BlendScanline(unsigned int row, unsigned int x_ofs, unsigned int w, const void* data);
	void FillScanline(unsigned int row, unsigned int x, unsigned int w, uint32_t colour);
	const uint32_t* GetScanline(unsigned int row, unsigned int x_ofs) const;
	
	CRect	m_rect;
	uint32_t*	m_data;
private:
	int	m_fd;
};

};	// namespace AxWin

#endif

