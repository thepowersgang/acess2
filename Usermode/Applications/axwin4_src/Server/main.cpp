/*
 * Acess2 GUI v4 (AxWin4)
 * - By John Hodge (thePowesGang)
 * 
 * main.cpp
 * - Program main
 */
#include <CConfig.hpp>
#include <ipc.hpp>
#include <input.hpp>
#include <video.hpp>
#include <CCompositor.hpp>
#include <timing.hpp>
#include <exception>
#include <algorithm>
#include <common.hpp>

extern "C" {
#include <stdio.h>
#include <stdint.h>
};

using namespace AxWin;

// === CODE ===
int main(int argc, char *argv[])
{
	// - Load configuration (from file and argv)
	CConfig	config;
	try {
		config.parseCommandline(argc, argv);
	}
	catch(const std::exception& e) {
		fprintf(stderr, "Exception: %s\n", e.what());
		return 1;
	}
	// - Open graphics
	CVideo* vid = new CVideo(config.m_video);
	// - Initialise compositor structures
	CCompositor* compositor = new CCompositor(/*config.m_compositor,*/ *vid);
	// - Open input
	Input::Initialise(config.m_input);
	//  > Handles hotkeys?
	// - Bind IPC channels
	IPC::Initialise(config.m_ipc, compositor);
	// - Start root child process (from config)
	// TODO: Spin up child process

	// - Event loop
	for( ;; )
	{
		 int	nfd = 0;
		fd_set	rfds;
		
		nfd = ::std::max(nfd, Input::FillSelect(rfds));
		nfd = ::std::max(nfd, IPC::FillSelect(rfds));
		
		// TODO: Support _SysSendMessage IPC?
		int64_t	timeout = Timing::GetTimeToNextEvent();
		int rv = ::_SysSelect(nfd, &rfds, NULL, &rfds, &timeout, 0);
		
		Timing::CheckEvents();
		
		Input::HandleSelect(rfds);
		IPC::HandleSelect(rfds);
		
		compositor->Redraw();
	}
	return 0;
}

namespace AxWin {

const char* InitFailure::what() const throw()
{
	return m_what;
}


}

