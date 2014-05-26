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

namespace AxWin {

extern "C" void AxWin4_DrawBitmap(tAxWin4_Window *Window, int X, int Y, unsigned int W, unsigned int H, void *Data)
{
	// TODO: Split message up into blocks such that it can be dispatched
	if( W > 128 )
	{
		const uint32_t*	data32 = static_cast<uint32_t*>(Data);
		for( unsigned int row = 0; row < H; row ++ )
		{
			CSerialiser	message;
			message.WriteU8(IPCMSG_PUSHDATA);
			message.WriteU16(Window->m_id);
			message.WriteU16(X);
			message.WriteU16(Y+row);
			message.WriteU16(W);
			message.WriteU16(1);
			message.WriteBuffer(W*4, data32);
			::AxWin::SendMessage(message);
			data32 += W;
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

