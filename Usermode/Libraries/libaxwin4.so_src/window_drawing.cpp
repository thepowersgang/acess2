/*
 * AxWin4 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * window_drawing.cpp
 * - Window drawing code
 */
#include <axwin4/axwin.h>
#include "include/common.hpp"
#include <ipc_proto.hpp>
#include <algorithm>	// min

namespace AxWin {

void _push_data(tAxWin4_Window *Window, int X, int Y, unsigned int W, unsigned int H, const void *Data)
{
	CSerialiser	message;
	//_SysDebug("_push_data - (%i,%i), %ix%i %p", X, Y, W, H, Data);
	message.WriteU8(IPCMSG_PUSHDATA);
	message.WriteU16(Window->m_id);
	message.WriteU16(X);
	message.WriteU16(Y);
	message.WriteU16(W);
	message.WriteU16(H);
	message.WriteBuffer(H*W*4, Data);
	::AxWin::SendMessage(message);
}

extern "C" void AxWin4_DrawBitmap(tAxWin4_Window *Window, int X, int Y, unsigned int W, unsigned int H, void *Data)
{
	// TODO: Split message up into blocks such that it can be dispatched
	if( W > 128 )
	{
		const uint32_t*	data32 = static_cast<uint32_t*>(Data);
		const unsigned int rows_per_message = 1;	// 2048 / W;
		for( unsigned int row = 0; row < H; row += rows_per_message )
		{
			_push_data(Window, X, Y+row, W, ::std::min(rows_per_message,H-row), data32);
			data32 += W*rows_per_message;
		}
	}
	else
	{
		_push_data(Window, X, Y, W, H, Data);
	}
}

extern "C" void AxWin4_DrawControl(tAxWin4_Window *Window, int X, int Y, unsigned int W, unsigned int H, uint16_t ID, unsigned int Frame)
{
	CSerialiser	message;
	message.WriteU8(IPCMSG_DRAWCTL);
	message.WriteU16(Window->m_id);
	message.WriteU16(X);
	message.WriteU16(Y);
	message.WriteU16(W);
	message.WriteU16(H);
	message.WriteU16(ID);
	message.WriteU16(Frame);
	::AxWin::SendMessage(message);
}

extern "C" void AxWin4_DrawText(tAxWin4_Window *Window, int X, int Y, unsigned int W, unsigned int H, uint16_t FontID, const char *String)
{
	CSerialiser	message;
	message.WriteU8(IPCMSG_DRAWTEXT);
	message.WriteU16(Window->m_id);
	message.WriteU16(X);
	message.WriteU16(Y);
	message.WriteU16(W);
	message.WriteU16(H);
	message.WriteU16(FontID);
	message.WriteString(String);
	::AxWin::SendMessage(message);
}

};	// namespace AxWin

