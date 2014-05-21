/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * video.cpp
 * - Graphics output
 */
#include <cstddef>
#include <video.hpp>
#include <common.hpp>

extern "C" {
#include <acess/sys.h>
#include <acess/devices/pty.h>
}

namespace AxWin {

CVideo::CVideo(const CConfigVideo& config):
	m_fd(0)
{
	{
		if( _SysIOCtl(m_fd, DRV_IOCTL_TYPE, NULL) != DRV_TYPE_TERMINAL )
			throw AxWin::InitFailure("stdin isn't a terminal");
		struct ptydims	dims;
		if( _SysIOCtl(m_fd, PTY_IOCTL_GETDIMS, &dims) == -1 )
			throw AxWin::InitFailure("Failed to get dimensions from stdin");
		m_width = dims.PW;
		m_height = dims.PH;
		if( m_width == 0 || m_height == 0 )
			throw AxWin::InitFailure("Terminal not capable of graphics");
	}
}

void CVideo::BlitLine(const uint32_t* src, unsigned int dst_y, unsigned int dst_x, unsigned int width)
{
	//_SysWriteAt(m_fd, (dst_y * m_width + dst_x) * 4, width * 4, src);
	_SysSeek(m_fd, (dst_y * m_width + dst_x) * 4, SEEK_SET);
	_SysWrite(m_fd, src, width * 4);
}

};	// namespace AxWin

