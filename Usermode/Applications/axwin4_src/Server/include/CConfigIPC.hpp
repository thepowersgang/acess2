/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * CConfigIPC.hpp
 * - Configuration class
 */
#ifndef _CCONFIGIPC_H_
#define _CCONFIGIPC_H_

#include <string>

namespace AxWin {

class CConfigIPC_Channel
{
public:
	::std::string	m_name;
	::std::string	m_argument;
};

class CConfigIPC
{
public:
	CConfigIPC();
};

};	// namespace AxWin

#endif

