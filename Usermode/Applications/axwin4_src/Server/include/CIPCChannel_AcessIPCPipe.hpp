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
#include <string>
#include <list>

namespace AxWin {

class CClient_AcessIPCPipe
{
public:
};

class CIPCChannel_AcessIPCPipe:
	public IIPCChannel
{
	 int	m_mainFD;
	::std::list<CClient_AcessIPCPipe>	m_clients;
public:
	CIPCChannel_AcessIPCPipe(const ::std::string& suffix);
	virtual ~CIPCChannel_AcessIPCPipe();
	
	virtual int  FillSelect(fd_set& rfds);
	virtual void HandleSelect(const fd_set& rfds);
};

}	// namespace AxWin

#endif

