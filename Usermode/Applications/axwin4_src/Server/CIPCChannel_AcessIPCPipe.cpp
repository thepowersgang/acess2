/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CIPCChannel_AcessIPCPipe.cpp
 * - IPC Channel :: Acess' IPC Pipe /Devices/ipcpipe/<name>
 */
#include <ipc.hpp>
#include <CIPCChannel_AcessIPCPipe.hpp>
#include <cerrno>
#include <system_error>
#include <acess/sys.h>
#include <algorithm>

namespace AxWin {

CIPCChannel_AcessIPCPipe::CIPCChannel_AcessIPCPipe(const ::std::string& suffix)
{
	::std::string	path = "/Devices/ipcpipe/" + suffix;
	m_fd = _SysOpen(path.c_str(), OPENFLAG_CREATE);
	if(m_fd == -1) {
		_SysDebug("Failed to open %s: %s", path.c_str(), strerror(errno));
		throw ::std::system_error(errno, ::std::system_category());
	}
}
CIPCChannel_AcessIPCPipe::~CIPCChannel_AcessIPCPipe()
{
	_SysClose(m_fd);
}

int CIPCChannel_AcessIPCPipe::FillSelect(fd_set& rfds)
{
	 int	maxfd = m_fd;
	FD_SET(m_fd, &rfds);
	
	for( auto& clientref : m_clients )
	{
		maxfd = ::std::max(maxfd, clientref.m_fd);
		FD_SET(clientref.m_fd, &rfds);
	}
	
	return maxfd+1;
}

void CIPCChannel_AcessIPCPipe::HandleSelect(const fd_set& rfds)
{
	if( FD_ISSET(m_fd, &rfds) )
	{
		int newfd = _SysOpenChild(m_fd, "newclient", OPENFLAG_READ|OPENFLAG_WRITE);
		if( newfd == -1 ) {
			_SysDebug("ERROR - Failure to open new client on FD%i", m_fd);
		}
		else {
			_SysDebug("CIPCChannel_AcessIPCPipe::HandleSelect - New client on FD %i with FD%i",
				m_fd, newfd);
			
			// emplace creates a new object within the list
			m_clients.emplace( m_clients.end(), *this, newfd );
			IPC::RegisterClient( m_clients.back() );
		}
	}

	for( auto it = m_clients.begin(); it != m_clients.end();  )
	{
		CClient_AcessIPCPipe& clientref = *it;
		++ it;
		
		if( FD_ISSET(clientref.m_fd, &rfds) )
		{
			try {
				clientref.HandleReceive();
			}
			catch( const ::std::exception& e ) {
				_SysDebug("ERROR - Exception processing IPCPipe FD%i: '%s', removing",
					clientref.m_fd, e.what()
					);
				it = m_clients.erase(--it);
			}
		}
	}
}


CClient_AcessIPCPipe::CClient_AcessIPCPipe(::AxWin::IIPCChannel& channel, int fd):
	CClient(channel),
	m_fd(fd)
{
}

CClient_AcessIPCPipe::~CClient_AcessIPCPipe()
{
	_SysClose(m_fd);
	_SysDebug("Closed client FD%i", m_fd);
}

void CClient_AcessIPCPipe::SendMessage(CSerialiser& message)
{
	const ::std::vector<uint8_t>&	data = message.Compact();
	
	_SysDebug("CClient_AcessIPCPipe::SendMessage - %i bytes to %i", data.size(), m_fd);
	//_SysDebugHex("CClient_AcessIPCPipe::SendMessage", data.data(), data.size());
	_SysWrite(m_fd, data.data(), data.size());
}

void CClient_AcessIPCPipe::HandleReceive()
{
	::std::vector<uint8_t>	rxbuf(0x1000);
	size_t len = _SysRead(m_fd, rxbuf.data(), rxbuf.capacity());
	if( len == (size_t)-1 )
		throw ::std::system_error(errno, ::std::system_category());
	//_SysDebug("CClient_AcessIPCPipe::HandleReceive - Rx %i/%i bytes", len, rxbuf.capacity());
	//_SysDebugHex("CClient_AcessIPCPipe::HandleReceive", rxbuf.data(), len);
	rxbuf.resize(len);
	
	CDeserialiser	msg( ::std::move(rxbuf) );
	CClient::HandleMessage( msg );
}

};

