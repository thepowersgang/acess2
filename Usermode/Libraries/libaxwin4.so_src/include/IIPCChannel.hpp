/*
 * Acess2 GUIv4 Library
 * - By John Hodge (thePowersGang)
 *
 * IIPCChannel.h
 * - IPC Channel interface
 */
#ifndef _LIBAXWIN4_IIPCCHANNEL_H_
#define _LIBAXWIN4_IIPCCHANNEL_H_

#include <serialisation.hpp>

#include <acess/sys.h>

namespace AxWin {

class IIPCChannel
{
public:
	virtual ~IIPCChannel();
	virtual int	FillSelect(fd_set& fds) = 0;
	/**
	 * \return False if the connection has dropped/errored
	 */
	virtual bool	HandleSelect(const fd_set& fds) = 0;
	
	virtual void	Send(CSerialiser& message) = 0;
};

};	// namespace AxWin

#endif

