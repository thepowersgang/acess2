/*
 */
#include <ipc.hpp>
#include <input.hpp>
#include <timing.hpp>

using namespace AxWin;

// === CODE ===
int main(int argc, char *argv[])
{
	// - Load configuration (from file and argv)
	// - Open graphics
	Graphics::Open(/*config.m_displayDevice*/);
	// - Open input
	// - Initialise compositor structures
	// - Prepare global hotkeys
	// - Bind IPC channels
	// - Start root child process (from config)
	
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
	}
	return 0;
}

