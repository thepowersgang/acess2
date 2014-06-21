/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CIPCChannel_AcessIPCPipe.hpp
 * - IPC Channel :: Acess' IPC Pipe /Devices/ipcpipe/<name>
 */
#ifndef _CIPCCHANNEL_ACESSIPCPIPE_HPP_
#define _CIPCCHANNEL_ACESSIPCPIPE_HPP_

#include <IIPCChannel.hpp>
#include <CClient.hpp>
#include <string>
#include <list>

namespace AxWin {

class CClient_AcessIPCPipe:
	public CClient
{
	friend class CIPCChannel_AcessIPCPipe;
	 int	m_fd;
public:
	CClient_AcessIPCPipe(IIPCChannel& channel, int fd);
	~CClient_AcessIPCPipe();
	
	void SendMessage(CSerialiser& message);
	
	void HandleReceive();
};

class CIPCChannel_AcessIPCPipe:
	public IIPCChannel
{
	 int	m_fd;
	::std::list<CClient_AcessIPCPipe>	m_clients;
public:
	CIPCChannel_AcessIPCPipe(const ::std::string& suffix);
	virtual ~CIPCChannel_AcessIPCPipe();
	
	virtual int  FillSelect(fd_set& rfds);
	virtual void HandleSelect(const fd_set& rfds);
};

}	// namespace AxWin

#endif

