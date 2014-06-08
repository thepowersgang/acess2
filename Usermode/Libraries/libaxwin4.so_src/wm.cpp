/*
 * AxWin4 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * wm.cpp
 * - Window Management
 */
#include <axwin4/axwin.h>
#include "include/common.hpp"
#include <ipc_proto.hpp>

namespace AxWin {

extern "C" tAxWin4_Window *AxWin4_CreateWindow(const char *Name)
{
	// Allocate a window ID
	
	// Create window structure locally
	tAxWin4_Window *ret = new tAxWin4_Window();
	ret->m_id = 0;
	// Request creation of window
	CSerialiser	message;
	message.WriteU8(IPCMSG_CREATEWIN);
	message.WriteU16(ret->m_id);
	message.WriteString(Name);
	::AxWin::SendMessage(message);
	return ret;
}

extern "C" void AxWin4_ShowWindow(tAxWin4_Window *Window)
{
	CSerialiser	message;
	message.WriteU8(IPCMSG_SETWINATTR);
	message.WriteU16(Window->m_id);
	message.WriteU16(IPC_WINATTR_SHOW);
	message.WriteU8(1);
	::AxWin::SendMessage(message);
}

extern "C" void AxWin4_MoveWindow(tAxWin4_Window *Window, int X, int Y)
{
	CSerialiser	message;
	message.WriteU8(IPCMSG_SETWINATTR);
	message.WriteU16(Window->m_id);
	message.WriteU16(IPC_WINATTR_POSITION);
	message.WriteS16(X);
	message.WriteS16(Y);
	::AxWin::SendMessage(message);
}
extern "C" void AxWin4_ResizeWindow(tAxWin4_Window *Window, unsigned int W, unsigned int H)
{
	CSerialiser	message;
	message.WriteU8(IPCMSG_SETWINATTR);
	message.WriteU16(Window->m_id);
	message.WriteU16(IPC_WINATTR_DIMENSIONS);
	message.WriteU16(W);
	message.WriteU16(H);
	::AxWin::SendMessage(message);
}

extern "C" void AxWin4_SetTitle(tAxWin4_Window *Window, const char *Title)
{
	CSerialiser	message;
	message.WriteU8(IPCMSG_SETWINATTR);
	message.WriteU16(Window->m_id);
	message.WriteU16(IPC_WINATTR_TITLE);
	message.WriteString(Title);
	::AxWin::SendMessage(message);
}

extern "C" void *AxWin4_GetWindowBuffer(tAxWin4_Window *Window)
{
	//if( !Window->m_buffer )
	//{
	//	// TODO: Support non-blocking operations	
	//}
	return NULL;
}

};	// namespace AxWin

