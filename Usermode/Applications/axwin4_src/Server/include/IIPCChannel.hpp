/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * IIPCChannel.hpp
 * - IPC Channel interface
 */
#ifndef _IIPCCHANNEL_H_
#define _IIPCCHANNEL_H_

namespace AxWin {

class IIPCChannel
{
public:
	virtual ~IIPCChannel();
	
	virtual int	FillSelect(::fd_set& rfds) = 0;
	virtual void	HandleSelect(::fd_set& rfds) = 0;
};


};

#endif

