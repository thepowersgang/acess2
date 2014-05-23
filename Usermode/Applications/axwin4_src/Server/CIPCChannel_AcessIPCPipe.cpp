/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CIPCChannel_AcessIPCPipe.cpp
 * - IPC Channel :: Acess' IPC Pipe /Devices/ipcpipe/<name>
 */
#include <CIPCChannel_AcessIPCPipe.hpp>

namespace AxWin {

CIPCChannel_AcessIPCPipe::CIPCChannel_AcessIPCPipe(const ::std::string& suffix)
{
	
}
CIPCChannel_AcessIPCPipe::~CIPCChannel_AcessIPCPipe()
{
}

int CIPCChannel_AcessIPCPipe::FillSelect(fd_set& rfds)
{
	return 0;
}

void CIPCChannel_AcessIPCPipe::HandleSelect(const fd_set& rfds)
{
}

};

