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
	_SysDebug("CIPCChannel_AcessIPCPipe::FillSelect");
	 int	maxfd = m_fd;
	FD_SET(m_fd, &rfds);
	
	for( auto& clientref : m_clients )
	{
		_SysDebug("CIPCChannel_AcessIPCPipe::FillSelect - FD%i", clientref.m_fd);
		maxfd = ::std::max(maxfd, clientref.m_fd);
		FD_SET(clientref.m_fd, &rfds);
	}
	
	return maxfd+1;
}

void CIPCChannel_AcessIPCPipe::HandleSelect(const fd_set& rfds)
{
	_SysDebug("CIPCChannel_AcessIPCPipe::HandleSelect");
	if( FD_ISSET(m_fd, &rfds) )
	{
		_SysDebug("CIPCChannel_AcessIPCPipe::HandleSelect - New client on FD %i", m_fd);
		int newfd = _SysOpenChild(m_fd, "newclient", OPENFLAG_READ|OPENFLAG_WRITE);
		_SysDebug("newfd = %i", newfd);
		
		// emplace creates a new object within the list
		m_clients.emplace( m_clients.end(), *this, newfd );
		IPC::RegisterClient( m_clients.back() );
	}

	for( auto it = m_clients.begin(); it != m_clients.end();  )
	{
		CClient_AcessIPCPipe& clientref = *it;
		_SysDebug("CIPCChannel_AcessIPCPipe::HandleSelect - Trying FD%i", clientref.m_fd);
		++ it;
		
		if( FD_ISSET(clientref.m_fd, &rfds) )
		{
			try {
				clientref.HandleReceive();
			}
			catch( const ::std::exception& e ) {
				_SysDebug("ERROR - Exception processing IPCPipe FD%i: %s",
					clientref.m_fd, e.what()
					);
				it = m_clients.erase(--it);
			}
		}
	}
	_SysDebug("CIPCChannel_AcessIPCPipe::HandleSelect - END");
}


CClient_AcessIPCPipe::CClient_AcessIPCPipe(::AxWin::IIPCChannel& channel, int fd):
	CClient(channel),
	m_fd(fd),
	m_rxbuf(0x1000)
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
	_SysWrite(m_fd, data.data(), data.size());
}

void CClient_AcessIPCPipe::HandleReceive()
{
	size_t len = _SysRead(m_fd, &m_rxbuf[0], m_rxbuf.capacity());
	if( len == (size_t)-1 )
		throw ::std::system_error(errno, ::std::system_category());
	
	CDeserialiser	message(len, &m_rxbuf[0]);
	CClient::HandleMessage(message);
}

};

