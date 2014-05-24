/*
 * Acess2 GUIv4 Library
 * - By John Hodge (thePowersGang)
 *
 * CIPCChannel_AcessIPCPipe.h
 * - Acess IPC Datagram pipe
 */
#ifndef _LIBAXWIN4_CIPCCHANNEL_ACESSIPCPIPE_H_
#define _LIBAXWIN4_CIPCCHANNEL_ACESSIPCPIPE_H_

#include "IIPCChannel.hpp"

namespace AxWin {

class CIPCChannel_AcessIPCPipe:
	public IIPCChannel
{
	 int	m_fd;
public:
	CIPCChannel_AcessIPCPipe(const char *Path);
	virtual ~CIPCChannel_AcessIPCPipe();
	virtual int	FillSelect(fd_set& fds);
	virtual bool	HandleSelect(const fd_set& fds);
	virtual void	Send(CSerialiser& message);
};

};	// namespace AxWin

#endif

