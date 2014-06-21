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

#include <system_error>
#include <cerrno>

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
		_SysDebug("Config threw exception: %s", e.what());
		return 1;
	}
	
	CVideo*	vid = 0;
	CCompositor*	compositor = 0;
	CInput*	input = 0;
	
	try {
		// - Open graphics
		vid = new CVideo(config.m_video);
		::_SysDebug("vid = %p", vid);
		// - Initialise compositor structures
		compositor = new CCompositor(/*config.m_compositor,*/ *vid);
		::_SysDebug("compositor = %p", compositor);
		// - Open input
		input = new CInput(config.m_input, *compositor);
		::_SysDebug("input = %p", input);
		//  > Handles hotkeys?
		// - Bind IPC channels
		IPC::Initialise(config.m_ipc, *compositor);
		::_SysDebug("IPC up");
		// - Start root child process (from config)
		// TODO: Spin up child process
		
		{
			const char *interface_app = "/Acess/Apps/AxWin/4.0/AxWinUI";
			const char *argv[] = {interface_app, NULL};
			 int	rv = _SysSpawn(interface_app, argv, NULL, 0, NULL, NULL);
			if( rv < 0 ) {
				_SysDebug("_SysSpawn chucked a sad, rv=%i, errno=%i", rv, _errno);
				throw ::std::system_error(errno, ::std::system_category());
			}
		}
	}
	catch(const ::std::exception& e) {
		fprintf(stderr, "Startup threw exception: %s\n", e.what());
		_SysDebug("Startup threw exception: %s", e.what());
		delete input;
		delete compositor;
		delete vid;
		return 1;
	}
	
	// - Event loop
	for( ;; )
	{
		 int	nfd = 0;
		fd_set	rfds, efds;
		
		FD_ZERO(&rfds);
		FD_ZERO(&efds);
	
		nfd = ::std::max(nfd, input->FillSelect(rfds));
		nfd = ::std::max(nfd, IPC::FillSelect(rfds));
		
		for( int i = 0; i < nfd; i ++ )
			if( FD_ISSET(i, &rfds) )
				FD_SET(i, &efds);

		#if 0
		for( int i = 0; i < nfd; i ++ ) {
			if( FD_ISSET(i, &rfds) ) {
				_SysDebug("FD%i", i);
			}
		}
		#endif
		
		// TODO: Support _SysSendMessage IPC?
		int64_t	timeout = Timing::GetTimeToNextEvent();
		int64_t	*timeoutp;
		if( timeout >= 0 ) {
			::_SysDebug("Calling select with timeout %lli", timeout);
			timeoutp = &timeout;
		}
		else {
			//::_SysDebug("Calling select with no timeout");
			timeoutp = 0;
		}
		int rv = ::_SysSelect(nfd, &rfds, NULL, NULL/*&efds*/, timeoutp, 0);
		
		#if 0
		for( int i = 0; i < nfd; i ++ ) {
			if( FD_ISSET(i, &rfds) ) {
				_SysDebug("FD%i", i);
			}
		}
		#endif
		//_SysDebug("rv=%i, timeout=%lli", rv, timeout);
		
		try {
			Timing::CheckEvents();
			
			input->HandleSelect(rfds);
			IPC::HandleSelect(rfds);
			
			compositor->Redraw();
		}
		catch(...) {
			::_SysDebug("Exception during select handling");
		}
	}
	return 0;
}

namespace AxWin {

const char* InitFailure::what() const throw()
{
	return m_what;
}


}

