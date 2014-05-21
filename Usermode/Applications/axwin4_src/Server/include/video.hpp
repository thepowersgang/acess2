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
public:
	CVideo(const CConfigVideo& config);
	
	void BlitLine(const uint32_t* src, unsigned int dst_y, unsigned int dst_x, unsigned int width);
};

};

#endif

