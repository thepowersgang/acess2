/*
 * AxWin4 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * ipc_acessipcpipe.c
 * - Acess2 /Devices/ipcpipe/ IPC Channel
 */
#include "include/common.hpp"
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
		::std::vector<uint8_t>	rxbuf(4096);
		size_t len = _SysRead(m_fd, rxbuf.data(), rxbuf.capacity());
		if( len == (size_t)-1 )
			throw ::std::system_error(errno, ::std::system_category());
		rxbuf.resize(len);
		_SysDebug("CClient_AcessIPCPipe::HandleReceive - Rx %i/%i bytes", len, rxbuf.capacity());
		_SysDebugHex("CClient_AcessIPCPipe::HandleReceive", rxbuf.data(), len);
		
		CDeserialiser	msg(rxbuf);
		::AxWin::RecvMessage( msg );
	}
	return true;
}

void CIPCChannel_AcessIPCPipe::Send(CSerialiser& message)
{
	const ::std::vector<uint8_t>& serialised = message.Compact();
	if(serialised.size() > 0x1000 ) {
		throw ::std::length_error("CIPCChannel_AcessIPCPipe::Send");
	}
	_SysDebug("CIPCChannel_AcessIPCPipe::Send(%i bytes)", serialised.size());
	//_SysDebugHex("CIPCChannel_AcessIPCPipe::Send",     serialised.data(), serialised.size());
	
	size_t rv = _SysWrite(m_fd, serialised.data(), serialised.size());
	if( rv != serialised.size() ) {
		throw ::std::system_error(errno, ::std::system_category());
	}
}


};	// namespace AxWin

