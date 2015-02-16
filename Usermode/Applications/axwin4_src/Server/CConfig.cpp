/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CConfig.cpp
 * - Configuration
 */
#include <CConfig.hpp>

namespace AxWin {

CConfig::CConfig()
{
}

bool CConfig::parseCommandline(int argc, char *argv[])
{
	return false;
}

CConfigVideo::CConfigVideo()
{
}

CConfigInput::CConfigInput():
	mouse_device("/Devices/Mouse/system")
{
}

CConfigIPC::CConfigIPC()
{
}

};	// namespace AxWin

