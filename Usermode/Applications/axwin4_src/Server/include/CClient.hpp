/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * CClient.hpp
 * - IPC Client
 */
#ifndef _CCLIENT_H_
#define _CCLIENT_H_

#include "CWindow.hpp"
#include "serialisation.hpp"

class IIPCChannel;

namespace AxWin {

class CClient
{
	IIPCChannel&	m_channel;
	
	//::std::map<unsigned int,CWindow*>	m_windows;
	CWindow*	m_windows[1];
public:
	CClient(IIPCChannel& channel);
	~CClient();
	
	CWindow*	GetWindow(int ID);
	void	SetWindow(int ID, CWindow* window);
	
	void	SendMessage(CSerialiser& reply);
};


};

#endif

