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
#include <string.h>
#include "include/lowlevel.h"

// === IMPORTS ===
extern void	Video_Setup(void);
extern void	WM_Initialise(void);
extern void	WM_Update(void);

// === PROTOTYPES ===
void	ParseCommandline(int argc, char **argv);

// === GLOBALS ===
const char	*gsTerminalDevice = NULL;
const char	*gsMouseDevice = NULL;

 int	giScreenWidth = 640;
 int	giScreenHeight = 480;

 int	giTerminalFD = -1;
 int	giTerminalFD_Input = 0;
 int	giMouseFD = -1;

#define __INSTALL_ROOT	"/Acess/Apps/AxWin/3.0"

const char	*gsInstallRoot = __INSTALL_ROOT;
const char	*gsInterfaceApp = __INSTALL_ROOT"/AxWinUI";
 int	gbNoSpawnUI = 0;

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
		gsMouseDevice = "/Devices/Mouse/system";
	}
	
	Video_Setup();
	IPC_Init();
	Input_Init();
	
	WM_Initialise();
	
	// Spawn interface root
	if( !gbNoSpawnUI )
	{
	 	int	server_tid = gettid();
		_SysDebug("server_tid = %i", server_tid);
		char	server_info[] = "AXWIN3_SERVER=00000";
		const char	*envp[] = {server_info, NULL};
		const char	*argv[] = {gsInterfaceApp, NULL};
		_SysDebug("server_tid = %i, &server_tid = %p", server_tid, &server_tid);
		sprintf(server_info, "AXWIN3_SERVER=%i", server_tid);
		// TODO: Does the client need FDs?
		 int	rv = _SysSpawn(gsInterfaceApp, argv, envp, 0, NULL, NULL);
		if( rv < 0 ) {
			_SysDebug("_SysSpawn chucked a sad, rv=%i, errno=%i", rv, _errno);
		}
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

void PrintUsage(void)
{
	fprintf(stderr, "Arguments:\n");
	fprintf(stderr, "  --no-ui  : Don't spawn the UI process\n");
}

void ParseCommandline(int argc, char **argv)
{
	for( int i = 1; i < argc; i ++ )
	{
		if( argv[i][0] != '-' ) {
			// Error?
			PrintUsage();
			exit(-1);
		}
		else if( argv[i][1] != '-' ) {
			// Short
			PrintUsage();
			exit(-1);
		}
		else {
			// Long
			if( strcmp(argv[i], "--no-ui") == 0 ) {
				gbNoSpawnUI = 1;
			}
			else {
				// Error.
				PrintUsage();
				exit(-1);
			}
		}
	}
}

