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
#include <acess/devices/pty_cmds.h>
#include "resources/cursor.h"
}

namespace AxWin {

CVideo::CVideo(const CConfigVideo& config):
	m_fd(1),
	m_width(0),
	m_height(0),
	m_bufferFormat(PTYBUFFMT_TEXT)
{
	// Obtain dimensions
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
	
	_SysDebug("m_width=%i, m_height=%i", m_width, m_height);
	SetCursorBitmap();
	
	SetCursorPos( m_width/2, m_height/2 );
	
	SetBufFormat(PTYBUFFMT_FB);
	uint32_t	data[m_width];
	for( unsigned int i = 0; i < m_height; i ++ )
		BlitLine(data, i, 0, m_width);
}

void CVideo::GetDims(unsigned int& w, unsigned int& h)
{
	w = m_width;
	h = m_height;
}

void CVideo::BlitLine(const uint32_t* src, unsigned int dst_y, unsigned int dst_x, unsigned int width)
{
	//_SysDebug("CVideo::BlitLine (%p, %i, %i, %i)", src, dst_y, dst_x, width);
	size_t	cmdlen = sizeof(struct ptycmd_senddata)/4 + width;
	//_SysDebug(" - Offset = %i, cmdlen = %i", (dst_y * m_width + dst_x) * 4, cmdlen);
	struct ptycmd_senddata	cmd = {
		{PTY2D_CMD_SEND, cmdlen & 0xFF, cmdlen>>8},
		(dst_y * m_width + dst_x)
	};
	SetBufFormat(PTYBUFFMT_2DCMD);
	_SysWrite(m_fd, &cmd, sizeof(cmd));
	_SysWrite(m_fd, src, width*4);

	//SetBufFormat(PTYBUFFMT_FB);
	//_SysWriteAt(m_fd, (dst_y * m_width + dst_x) * 4, width * 4, src);
	//_SysSeek(m_fd, (dst_y * m_width + dst_x) * 4, SEEK_SET);
	//_SysWrite(m_fd, src, width * 4);
}

void CVideo::Flush()
{
	// TODO: Write to a local copy of the framebuffer in BlitLine, and then flush out in this function
}

void CVideo::SetBufFormat(unsigned int FormatID)
{
	if( m_bufferFormat != FormatID )
	{
		
		struct ptymode mode = {.OutputMode = FormatID, .InputMode = 0};
		int rv = _SysIOCtl( m_fd, PTY_IOCTL_SETMODE, &mode );
		if( rv ) {
			throw ::AxWin::InitFailure("Can't set PTY to framebuffer mode");
		}
		
		m_bufferFormat = FormatID;
	}
}

void CVideo::SetCursorBitmap()
{
	// Set cursor position and bitmap
	struct ptycmd_header	hdr = {PTY2D_CMD_SETCURSORBMP,0,0};
	size_t size = sizeof(hdr) + sizeof(cCursorBitmap) + cCursorBitmap.W*cCursorBitmap.H*4;
	hdr.len_low = size / 4;
	hdr.len_hi = size / (256*4);
	
	SetBufFormat(PTYBUFFMT_2DCMD);
	_SysWrite(m_fd, &hdr, sizeof(hdr));
	_SysDebug("SetCursorBitmap - size = %i (%04x:%02x * 4)", size, hdr.len_hi, hdr.len_low);
	_SysWrite(m_fd, &cCursorBitmap, size-sizeof(hdr));
}

void CVideo::SetCursorPos(int X, int Y)
{
	struct ptycmd_setcursorpos	cmd;
	cmd.hdr.cmd = PTY2D_CMD_SETCURSORPOS;
	cmd.hdr.len_low = sizeof(cmd)/4;
	cmd.hdr.len_hi = 0;
	cmd.x = X;
	cmd.y = Y;

	SetBufFormat(PTYBUFFMT_2DCMD);	
	_SysWrite(m_fd, &cmd, sizeof(cmd));
}

};	// namespace AxWin

