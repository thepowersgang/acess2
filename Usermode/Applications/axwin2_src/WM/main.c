/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"
#include <acess/sys.h>

// === IMPORTS ===
extern void	ParseCommandline(int argc, char *argv[]);
extern void	Video_Setup();

// === GLOBALS ===
char	*gsTerminalDevice = NULL;
char	*gsMouseDevice = NULL;

 int	giScreenWidth = 640;
 int	giScreenHeight = 480;
uint32_t	*gpScreenBuffer = NULL;

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
		gsTerminalDevice = "/Devices/VTerm/7";
	}
	
	Video_Setup();
	
	// Main Loop
	for(;;)
	{
		Messages_PollIPC();
		//yield();
	}
	return 0;
}
