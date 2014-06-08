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

extern "C" void AxWin4_DrawBitmap(tAxWin4_Window *Window, int X, int Y, unsigned int W, unsigned int H, void *Data)
{
	// TODO: Split message up into blocks such that it can be dispatched
	if( W > 128 )
	{
		const uint32_t*	data32 = static_cast<uint32_t*>(Data);
		const unsigned int rows_per_message = 2048 / W;
		for( unsigned int row = 0; row < H; row += rows_per_message )
		{
			CSerialiser	message;
			message.WriteU8(IPCMSG_PUSHDATA);
			message.WriteU16(Window->m_id);
			message.WriteU16(X);
			message.WriteU16(Y+row);
			message.WriteU16(W);
			message.WriteU16( ::std::min(rows_per_message,H-row) );
			message.WriteBuffer(W*4, data32);
			::AxWin::SendMessage(message);
			data32 += W*rows_per_message;
		}
	}
	else
	{
		CSerialiser	message;
		message.WriteU8(IPCMSG_PUSHDATA);
		message.WriteU16(Window->m_id);
		message.WriteU16(X);
		message.WriteU16(Y);
		message.WriteU16(W);
		message.WriteU16(H);
		message.WriteBuffer(W*H*4, Data);
		::AxWin::SendMessage(message);
	}
}

};	// namespace AxWin

