/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"
#include <acess/sys.h>

// === IMPORTS ===
extern void	ParseCommandline(int argc, char *argv[]);
extern void	Video_Setup(void);
extern void	WM_Update(void);
extern void	Interface_Init(void);
extern void	IPC_Init(void);
extern void	IPC_FillSelect(int *nfds, fd_set *set);
extern void	IPC_HandleSelect(fd_set *set);
extern void	Input_FillSelect(int *nfds, fd_set *set);
extern void	Input_HandleSelect(fd_set *set);

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
		gsTerminalDevice = "/Devices/VTerm/6";
	}
	if( gsMouseDevice == NULL ) {
		gsMouseDevice = "/Devices/PS2Mouse";
	}
	
	Video_Setup();
	Interface_Init();
	IPC_Init();
	
	WM_Update();
	
	// Main Loop
	for(;;)
	{
		fd_set	fds;
		 int	nfds = 0;
		FD_ZERO(&fds);
	
		Input_FillSelect(&nfds, &fds);
		IPC_FillSelect(&nfds, &fds);

		select(nfds, &fds, NULL, NULL, NULL);

		Input_HandleSelect(&fds);
		IPC_HandleSelect(&fds);
	}
	return 0;
}

