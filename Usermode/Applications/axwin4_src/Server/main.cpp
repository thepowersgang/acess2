/*
 */
#include <CConfig.hpp>
#include <ipc.hpp>
#include <input.hpp>
#include <video.hpp>
#include <timing.hpp>

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
	Graphics::Initialise(config.m_video);
	// - Open input
	Input::Initialise(config.m_input);
	//  > Handles hotkeys?
	// - Initialise compositor structures
	Compositor::Initialise(config.m_compositor);
	// - Bind IPC channels
	IPC::Initialise(config.m_ipc);
	// - Start root child process (from config)
	// TODO: Spin up child process

	// - Event loop
	for( ;; )
	{
		 int	nfd = 0;
		fd_set	rfds;
		
		Input::FillSelect(&nfd, &rfds);
		IPC::FillSelect(&nfd, &rfds);
		
		// TODO: Timer events
		int64_t	timeout = Timing::GetTimeToNextEvent();
		int rv = ::_SysSelect(nfd, &rfds, NULL, &rfds, NULL, 0);
		
		Timing::CheckEvents();
		
		Input::HandleSelect(&rfds);
		IPC::HandleSelect(&rfds);
		
		Compositor::Redraw();
	}
	return 0;
}

