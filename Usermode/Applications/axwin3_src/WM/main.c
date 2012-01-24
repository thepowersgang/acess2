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
extern void	Video_Setup(void);
extern void	WM_Initialise(void);
extern int	Renderer_Menu_Init(void);
extern int	Renderer_Widget_Init(void);
extern int	Renderer_Background_Init(void);
extern void	WM_Update(void);

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
	
	Renderer_Menu_Init();
	Renderer_Widget_Init();
	Renderer_Background_Init();
	WM_Initialise();
	
	// Spawn interface root
	if( clone(CLONE_VM, 0) == 0 )
	{
		static char csInterfaceApp[] = __INSTALL_ROOT"/AxWinUI";
		char	server_info[] = "AXWIN3_SERVER=00000";
		char	*envp[] = {server_info, NULL};
		char	*argv[] = {csInterfaceApp, NULL};
		_SysDebug("server_tid = %i, &server_tid = %p", server_tid, &server_tid);
		sprintf(server_info, "AXWIN3_SERVER=%i", server_tid);
		execve(csInterfaceApp, argv, envp);
		exit(1);
	}

	// Main Loop
	for(;;)
	{
		fd_set	fds;
		 int	nfds = 0;
		FD_ZERO(&fds);
	
		WM_Update();

		Input_FillSelect(&nfds, &fds);
		IPC_FillSelect(&nfds, &fds);
		
		nfds ++;
		if( _SysSelect(nfds, &fds, NULL, NULL, NULL, THREAD_EVENT_IPCMSG) == -1 ) {
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

