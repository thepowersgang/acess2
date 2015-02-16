/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * IIPCChannel.hpp
 * - IPC Channel interface
 */
#ifndef _IIPCCHANNEL_H_
#define _IIPCCHANNEL_H_

extern "C" {
#include <acess/sys.h>
}

namespace AxWin {

class IIPCChannel
{
public:
	virtual ~IIPCChannel();
	
	virtual int	FillSelect(::fd_set& rfds) = 0;
	virtual void	HandleSelect(const ::fd_set& rfds) = 0;
};


};

#endif

