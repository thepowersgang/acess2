/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CClient.cpp
 * - IPC Client
 */
#include <CClient.hpp>

namespace AxWin {

CClient::CClient(IIPCChannel& channel):
	m_channel(channel)
{
	
}

CWindow* CClient::GetWindow(int ID)
{
	if( ID == 0 )
		return 0;
	
	return m_windows[ID];
}

void CClient::SetWindow(int ID, CWindow* window)
{
	if( m_windows[ID] ) {
		delete m_windows[ID];
	}
	m_windows[ID] = window;
}

void CClient::SendMessage(CSerialiser& reply)
{
}

};	// namespace AxWin

