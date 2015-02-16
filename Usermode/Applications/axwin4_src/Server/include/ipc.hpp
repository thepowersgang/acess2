/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * ipc.hpp
 * - IPC Interface Header
 */
#ifndef _IPC_H_
#define _IPC_H_

#include <exception>

extern "C" {
#include <acess/sys.h>
};

#include <serialisation.hpp>
#include <CConfigIPC.hpp>

namespace AxWin {

class CCompositor;
class CClient;

namespace IPC {

extern void	Initialise(const CConfigIPC& config, CCompositor& compositor);
extern int	FillSelect(::fd_set& rfds);
extern void	HandleSelect(const ::fd_set& rfds);
extern void	RegisterClient(CClient& client);
extern CClient*	GetClientByID(uint16_t id);
extern void	DeregisterClient(CClient& client);

extern void	SendMessage_NotifyDims(CClient& client, unsigned int WinID, unsigned int NewW, unsigned int NewH);
extern void	SendMessage_MouseButton(CClient& client, unsigned int WinID, unsigned int X, unsigned int Y, uint8_t Button, bool Pressed);
extern void	SendMessage_MouseMove(CClient& client, unsigned int WinID, unsigned int X, unsigned int Y);
extern void	SendMessage_KeyEvent(CClient& client, unsigned int WinID, uint32_t KeySym, bool Pressed, const char *Translated);

extern void	HandleMessage(CClient& client, CDeserialiser& message);

class CClientFailure:
	public ::std::exception
{
	const std::string m_what;
public:
	CClientFailure(std::string&& what);
	~CClientFailure() throw();
	const char *what() const throw();
};

};	// namespace IPC
};	// namespace AxWin

#endif

