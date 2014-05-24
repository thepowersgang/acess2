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

#include <CConfigIPC.hpp>

namespace AxWin {
class CCompositor;

namespace IPC {

extern void	Initialise(const CConfigIPC& config, CCompositor& compositor);
extern int	FillSelect(::fd_set& rfds);
extern void	HandleSelect(::fd_set& rfds);

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

