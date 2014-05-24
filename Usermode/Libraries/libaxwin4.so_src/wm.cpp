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
	// Request creation of window
}

extern "C" void AxWin4_ShowWindow(tAxWin4_Window *Window)
{
	CSerialiser	message;
	message.WriteU16(Window->m_id);
	message.WriteU16(IPC_WINATTR_SHOW);
	message.WriteU8(1);
	::AxWin::SendMessage(message);
}

extern "C" void AxWin4_SetTitle(tAxWin4_Window *Window, const char *Title)
{
	CSerialiser	message;
	message.WriteU16(Window->m_id);
	message.WriteU16(IPC_WINATTR_TITLE);
	message.WriteString(Title);
	::AxWin::SendMessage(message);
}

};	// namespace AxWin

