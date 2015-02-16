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
#include <draw_text.hpp>	// for the fonts

namespace AxWin {

CClient::CClient(::AxWin::IIPCChannel& channel):
	m_channel(channel),
	m_id(0)
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
	//_SysDebug("SetWindow(ID=%i,window=%p)", ID, window);
	auto it = m_windows.find(ID);
	if( it != m_windows.end() ) {
		_SysDebug("CLIENT BUG: Window ID %i is already used by %p", ID, it->second);
	}
	else {
		m_windows[ID] = window;
	}
}

IFontFace& CClient::GetFont(unsigned int id)
{
	static CFontFallback	fallback_font;
	if( id == 0 ) {
		_SysDebug("GetFont: %i = %p", id, &fallback_font);
		return fallback_font;
	}
	assert(!"TODO: CClient::GetFont id != 0");
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

