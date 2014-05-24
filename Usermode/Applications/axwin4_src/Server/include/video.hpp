/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * video.hpp
 * - Graphics Interface Header
 */
#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <cstdint>
#include "CConfigVideo.hpp"

namespace AxWin {

class CVideo
{
	 int	m_fd;
	unsigned int	m_width;
	unsigned int	m_height;
	 int	m_bufferFormat;
public:
	CVideo(const CConfigVideo& config);

	void GetDims(unsigned int& w, unsigned int& h);	

	void BlitLine(const uint32_t* src, unsigned int dst_y, unsigned int dst_x, unsigned int width);
	void Flush();

private:
	void SetBufFormat(unsigned int FormatID);
	void SetCursorBitmap();
	void SetCursorPos(int X, int Y);
};

};

#endif

