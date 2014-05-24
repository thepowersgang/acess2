/*
 * AxWin4 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * ipc_acessipcpipe.c
 * - Acess2 /Devices/ipcpipe/ IPC Channel
 */
#include "include/CIPCChannel_AcessIPCPipe.hpp"
#include <system_error>
#include <cerrno>

namespace AxWin {

CIPCChannel_AcessIPCPipe::CIPCChannel_AcessIPCPipe(const char *Path)
{
	m_fd = _SysOpen(Path, OPENFLAG_READ|OPENFLAG_WRITE);
	if( m_fd == -1 ) {
		throw ::std::system_error(errno, ::std::system_category());
	}
}

CIPCChannel_AcessIPCPipe::~CIPCChannel_AcessIPCPipe()
{
}

int CIPCChannel_AcessIPCPipe::FillSelect(fd_set& fds)
{
	FD_SET(m_fd, &fds);
	return m_fd+1;
}

bool CIPCChannel_AcessIPCPipe::HandleSelect(const fd_set& fds)
{
	if( FD_ISSET(m_fd, &fds) )
	{
	}
	return true;
}

void CIPCChannel_AcessIPCPipe::Send(CSerialiser& message)
{
	// TODO:
}


};	// namespace AxWin

