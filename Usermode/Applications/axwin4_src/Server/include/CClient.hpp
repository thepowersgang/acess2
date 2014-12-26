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
#include <map>
#include <cassert>
#include "IFontFace.hpp"

namespace AxWin {

class IIPCChannel;

class CClient
{
	unsigned int	m_id;
	IIPCChannel&	m_channel;
	
	::std::map<unsigned int,CWindow*>	m_windows;
	//CWindow*	m_windows[1];
public:
	CClient(::AxWin::IIPCChannel& channel);
	virtual ~CClient();
	
	void set_id(unsigned int id) { assert(m_id == 0); m_id = id; }
	unsigned int id() const { return m_id; }
	
	CWindow*	GetWindow(int ID);
	void	SetWindow(int ID, CWindow* window);
	
	IFontFace&	GetFont(unsigned int id);
	
	virtual void	SendMessage(CSerialiser& reply) = 0;
	void	HandleMessage(CDeserialiser& message);
};


};

#endif

