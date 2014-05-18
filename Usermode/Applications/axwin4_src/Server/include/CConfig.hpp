/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * CConfig.hpp
 * - Configuration class
 */
#ifndef _CCONFIG_H_
#define _CCONFIG_H_

#include "CConfigInput.hpp"
#include "CConfigVideo.hpp"
#include "CConfigIPC.hpp"

namespace AxWin {

class CConfig
{
public:
	CConfig();
	
	bool parseCommandline(int argc, char *argv[]);
	
	CConfigInput	m_input;
	CConfigVideo	m_video;
	CConfigIPC	m_ipc;
};

}

#endif

