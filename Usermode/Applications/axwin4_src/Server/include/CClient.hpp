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

namespace AxWin {

class IIPCChannel;

class CClient
{
	IIPCChannel&	m_channel;
	
	//::std::map<unsigned int,CWindow*>	m_windows;
	CWindow*	m_windows[1];
public:
	CClient(::AxWin::IIPCChannel& channel);
	virtual ~CClient();
	
	CWindow*	GetWindow(int ID);
	void	SetWindow(int ID, CWindow* window);
	
	virtual void	SendMessage(CSerialiser& reply) = 0;
	void	HandleMessage(CDeserialiser& message);
};


};

#endif

