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
#include <ipc_proto.hpp>
#include <algorithm>
#include <mutex>
#include <stdexcept>

#include <cstring>
#include <cstdio>

namespace AxWin {

IIPCChannel*	gIPCChannel;
::std::mutex	glSyncReply;
bool	gSyncReplyActive;
bool	gSyncReplyValid;
CDeserialiser	gSyncReplyBuf;

extern "C" bool AxWin4_Connect(const char *URI)
{
	_SysDebug("AxWin4_Connect('%s')", URI);
	if( gIPCChannel ) {
		return false;
	}
	try {
		if( strncmp(URI, "ipcpipe://", 3+4+3) == 0 )
		{
			gIPCChannel = new CIPCChannel_AcessIPCPipe(URI+3+4+3);
		}
		else
		{
			_SysDebug("Unknown protocol");
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
	return AxWin4_WaitEventQueueSelect(0, NULL, NULL, NULL, Timeout);
}

extern "C" bool AxWin4_WaitEventQueueSelect(int nFDs, fd_set *rfds, fd_set *wfds, fd_set *efds, uint64_t Timeout)
{
	fd_set	local_rfds;
	if( !rfds ) {
		FD_ZERO(&local_rfds);
		rfds = &local_rfds;
	}
	
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
void RecvMessage(CDeserialiser& message)
{
	uint8_t id = message.ReadU8();
	_SysDebug("RecvMessage: id=%i", id);
	switch(id)
	{
	case IPCMSG_REPLY:
		// Flag reply and take a copy of this message
		if( !gSyncReplyActive )
		{
			_SysDebug("Unexpected reply message %i", message.ReadU8());
		}
		else if( gSyncReplyValid )
		{
			// Oh... that was unexpected
			_SysDebug("Unexpected second reply message %i", message.ReadU8());
		}
		else
		{
			gSyncReplyValid = true;
			gSyncReplyBuf = message;
		}
		break;
	default:
		_SysDebug("TODO: RecvMessage(%i)", id);
		break;
	}
}

CDeserialiser GetSyncReply(CSerialiser& request, unsigned int Message)
{
	::std::lock_guard<std::mutex>	lock(glSyncReply);
	gSyncReplyActive = true;
	gSyncReplyValid = false;
	// Send once lock is acquired
	SendMessage(request);
	
	while( !gSyncReplyValid )
	{
		// Tick along
		if( !AxWin4_WaitEventQueue(0) )
			throw ::std::runtime_error("Connection dropped while waiting for reply");
	}
	gSyncReplyActive = false;
	
	uint8_t id = gSyncReplyBuf.ReadU8();
	if( id != Message )
	{
		_SysDebug("Unexpected reply message id=%i, expected %i",
			id, Message);
		throw ::std::runtime_error("Sequencing error, unexpected reply");
	}
	
	// Use move to avoid copying
	return ::std::move(gSyncReplyBuf);
}

extern "C" void AxWin4_GetScreenDimensions(unsigned int ScreenIndex, unsigned int *Width, unsigned int *Height)
{
	CSerialiser	req;
	req.WriteU8(IPCMSG_GETGLOBAL);
	req.WriteU16(IPC_GLOBATTR_SCREENDIMS);
	req.WriteU8(ScreenIndex);
	
	CDeserialiser	response = GetSyncReply(req, IPCMSG_GETGLOBAL);
	if( response.ReadU16() != IPC_GLOBATTR_SCREENDIMS ) {
		
	}
	
	*Width = response.ReadU16();
	*Height = response.ReadU16();
	
	_SysDebug("AxWin4_GetScreenDimensions: %i = %ix%i", ScreenIndex, *Width, *Height);
}

IIPCChannel::~IIPCChannel()
{
}

};	// namespace AxWin

