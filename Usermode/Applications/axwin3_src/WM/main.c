/*
 * Acess2 GUI (AxWin) Version 3
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Entrypoint
 */
#include <common.h>
#include <acess/sys.h>

// === IMPORTS ===
extern void	WM_Update(void);
extern void	Video_Setup(void);

// === PROTOTYPES ===
void	ParseCommandline(int argc, char **argv);

// === GLOBALS ===
const char	*gsTerminalDevice = NULL;
const char	*gsMouseDevice = NULL;

 int	giScreenWidth = 640;
 int	giScreenHeight = 480;

 int	giTerminalFD = -1;
 int	giMouseFD = -1;
 

// === CODE ===
/**
 * \brief Program Entrypoint
 */
int main(int argc, char *argv[])
{
	ParseCommandline(argc, argv);
	
	if( gsTerminalDevice == NULL ) {
		gsTerminalDevice = "/Devices/VTerm/6";
	}
	if( gsMouseDevice == NULL ) {
		gsMouseDevice = "/Devices/PS2Mouse";
	}
	
	Video_Setup();
//	Interface_Init();
	IPC_Init();
	Input_Init();
	
//	WM_Update();
	
	// Main Loop
	for(;;)
	{
		fd_set	fds;
		 int	nfds = 0;
		FD_ZERO(&fds);
	
		Input_FillSelect(&nfds, &fds);
		IPC_FillSelect(&nfds, &fds);
		
		nfds ++;
		select(nfds, &fds, NULL, NULL, NULL);

		Input_HandleSelect(&fds);
		IPC_HandleSelect(&fds);
	}
	return 0;
}

void ParseCommandline(int argc, char **argv)
{
	
}

