/*
 * Acess2 GUI (AxWin) Version 3
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Entrypoint
 */
#include <common.h>
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>

// === IMPORTS ===
extern void	WM_Update(void);
extern void	Video_Setup(void);
extern int	Renderer_Widget_Init(void);

// === PROTOTYPES ===
void	ParseCommandline(int argc, char **argv);

// === GLOBALS ===
const char	*gsTerminalDevice = NULL;
const char	*gsMouseDevice = NULL;

 int	giScreenWidth = 640;
 int	giScreenHeight = 480;

 int	giTerminalFD = -1;
 int	giMouseFD = -1;

#define __INSTALL_ROOT	"/Acess/Apps/AxWin/3.0"

const char	*gsInstallRoot = __INSTALL_ROOT;

// === CODE ===
/**
 * \brief Program Entrypoint
 */
int main(int argc, char *argv[])
{
	 int	server_tid = gettid();
	
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
	
	Renderer_Widget_Init();
//	WM_Update();
	
	// Spawn interface root
	if( clone(CLONE_VM, 0) == 0 )
	{
		static char csInterfaceApp[] = __INSTALL_ROOT"/AxWinUI";
		char	server_info[] = "AXWIN3_SERVER=00000";
		char	*envp[] = {server_info, NULL};
		char	*argv[] = {csInterfaceApp, NULL};
		sprintf(server_info, "AXWIN3_SERVER=%i", server_tid);
		execve(csInterfaceApp, argv, envp);
	}

	// Main Loop
	for(;;)
	{
		fd_set	fds;
		 int	nfds = 0;
		FD_ZERO(&fds);
	
		Input_FillSelect(&nfds, &fds);
		IPC_FillSelect(&nfds, &fds);
		
		nfds ++;
		if( select(nfds, &fds, NULL, NULL, NULL) == -1 ) {
			_SysDebug("ERROR: select() returned -1");
			return -1;
		}

		Input_HandleSelect(&fds);
		IPC_HandleSelect(&fds);
	}
	return 0;
}

void ParseCommandline(int argc, char **argv)
{
	
}

