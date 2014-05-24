/*
 * AxWin4 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * ipc.c
 * - IPC Abstraction
 */
#include <axwin4/axwin.h>
#include "include/common.hpp"
#include "include/IIPCChannel.hpp"
#include "include/CIPCChannel_AcessIPCPipe.hpp"
#include <algorithm>

#include <cstring>
#include <cstdio>

namespace AxWin {

IIPCChannel*	gIPCChannel;

extern "C" bool AxWin4_Connect(const char *URI)
{
	if( gIPCChannel ) {
		return false;
	}
	try {
		if( strncmp(URI, "ipcpipe://", 3+4+3) == 0 )
		{
			gIPCChannel = new CIPCChannel_AcessIPCPipe(URI);
		}
		else
		{
			return false;
		}
	}
	catch( const ::std::exception& e )
	{
		fprintf(stderr, "AxWin4_Connect: %s\n", e.what());
		return false;
	}
	return true;
}

extern "C" bool AxWin4_PeekEventQueue(void)
{
	return false;
}

extern "C" bool AxWin4_WaitEventQueue(uint64_t Timeout)
{
	AxWin4_WaitEventQueueSelect(0, NULL, NULL, NULL, Timeout);
}

extern "C" bool AxWin4_WaitEventQueueSelect(int nFDs, fd_set *rfds, fd_set *wfds, fd_set *efds, uint64_t Timeout)
{
	fd_set	local_rfds;
	if( !rfds )
		rfds = &local_rfds;
	
	int64_t	select_timeout = Timeout;
	int64_t	*select_timeout_p = (Timeout ? &select_timeout : 0);
	
	nFDs = ::std::max(nFDs, gIPCChannel->FillSelect(*rfds));
	_SysSelect(nFDs, rfds, wfds, efds, select_timeout_p, 0);
	return gIPCChannel->HandleSelect(*rfds);
}

void SendMessage(CSerialiser& message)
{
	gIPCChannel->Send(message);
}

IIPCChannel::~IIPCChannel()
{
}

};	// namespace AxWin

