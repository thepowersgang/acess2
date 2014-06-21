/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CClient.cpp
 * - IPC Client
 */
#include <CClient.hpp>
#include <IIPCChannel.hpp>
#include <ipc.hpp>

namespace AxWin {

CClient::CClient(::AxWin::IIPCChannel& channel):
	m_channel(channel)
{
	
}

CClient::~CClient()
{
	::AxWin::IPC::DeregisterClient(*this);
}

CWindow* CClient::GetWindow(int ID)
{
	try {
		return m_windows.at(ID);
	}
	catch(const std::exception& e) {
		return NULL;
	}
}

void CClient::SetWindow(int ID, CWindow* window)
{
	_SysDebug("SetWindow(ID=%i,window=%p)", ID, window);
	if( m_windows[ID] ) {
		delete m_windows[ID];
	}
	m_windows[ID] = window;
}

void CClient::HandleMessage(CDeserialiser& message)
{
	try {
		IPC::HandleMessage(*this, message);
		if( !message.IsConsumed() )
		{
			_SysDebug("NOTICE - CClient::HandleMessage - Trailing data in message");
		}
	}
	catch( const ::std::exception& e )
	{
		_SysDebug("ERROR - Exception while processing message from client: %s", e.what());
	}
	catch( ... )
	{
		_SysDebug("ERROR - Unknown exception while processing message from client");
	}
}

};	// namespace AxWin

