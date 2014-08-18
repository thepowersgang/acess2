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
#include <vector>
#include <algorithm>
#include <mutex>

namespace AxWin {

static const int	MAX_WINDOW_ID = 16;
static ::std::mutex	glWindowList;
static ::std::vector<tAxWin4_Window*>	gWindowList;

extern "C" tAxWin4_Window *AxWin4_CreateWindow(const char *Name)
{
	// Allocate a window ID
	::std::lock_guard<std::mutex>	lock(glWindowList);
	int id = ::std::find(gWindowList.begin(), gWindowList.end(), nullptr) - gWindowList.end();
	if( id >= MAX_WINDOW_ID ) {
		throw ::std::runtime_error("AxWin4_CreateWindow - Out of IDs (TODO: Better exception)");
	}
	if( id == gWindowList.size() )
	{
		gWindowList.push_back(nullptr);
	}
	
	// Create window structure locally
	tAxWin4_Window *ret = new tAxWin4_Window();
	gWindowList[id] = ret;
	ret->m_id = id;
	ret->m_fd = -1;
	ret->m_buffer = nullptr;
	// Request creation of window
	CSerialiser	message;
	message.WriteU8(IPCMSG_CREATEWIN);
	message.WriteU16(ret->m_id);
	message.WriteString(Name);
	::AxWin::SendMessage(message);
	return ret;
}

extern "C" void AxWin4_ShowWindow(tAxWin4_Window *Window, bool Show)
{
	CSerialiser	message;
	message.WriteU8(IPCMSG_SETWINATTR);
	message.WriteU16(Window->m_id);
	message.WriteU16(IPC_WINATTR_SHOW);
	message.WriteU8( (Show ? 1 : 0) );
	::AxWin::SendMessage(message);
}

extern "C" void AxWin4_SetWindowFlags(tAxWin4_Window *Window, unsigned int Flags)
{
	CSerialiser	message;
	message.WriteU8(IPCMSG_SETWINATTR);
	message.WriteU16(Window->m_id);
	message.WriteU16(IPC_WINATTR_FLAGS);
	message.WriteU8( Flags );
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

extern "C" void AxWin4_DamageRect(tAxWin4_Window *Window, unsigned int X, unsigned int Y, unsigned int W, unsigned int H)
{
	CSerialiser	message;
	message.WriteU8(IPCMSG_DAMAGERECT);
	message.WriteU16(Window->m_id);
	message.WriteU16(X);
	message.WriteU16(Y);
	message.WriteU16(W);
	message.WriteU16(H);
	::AxWin::SendMessage(message);
}

extern "C" void *AxWin4_GetWindowBuffer(tAxWin4_Window *Window)
{
	if( Window->m_fd == -1 )
	{
		CSerialiser	req;
		req.WriteU8(IPCMSG_GETWINBUF);
		req.WriteU16(Window->m_id);
		
		CDeserialiser	response = GetSyncReply(req, IPCMSG_GETWINBUF);
		if( response.ReadU16() != Window->m_id )
		{
			
		}
		
		uint64_t handle = response.ReadU64();
		Window->m_fd = _SysUnMarshalFD(handle);
		
		_SysDebug("AxWin4_GetWindowBuffer: %llx = %i", handle, Window->m_fd);
	}

	if( !Window->m_buffer )
	{
		size_t	size = 640*480*4;
		Window->m_buffer = _SysMMap(NULL, size, MMAP_PROT_WRITE, MMAP_MAP_SHARED, Window->m_fd, 0);
	}

	return Window->m_buffer;
}

};	// namespace AxWin

