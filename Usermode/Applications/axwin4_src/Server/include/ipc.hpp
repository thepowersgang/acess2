/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * ipc.hpp
 * - IPC Interface Header
 */
#ifndef _IPC_H_
#define _IPC_H_

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

};	// namespace IPC
};	// namespace AxWin

#endif

