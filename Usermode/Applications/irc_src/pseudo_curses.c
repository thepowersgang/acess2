/*
 */
#include "pseudo_curses.h"
#include <stdbool.h>
#include <acess/sys.h>
#include <acess/devices/pty.h>
#include <stdio.h>

// === PROTOTYPES ===
bool	ACurses_GetDims_Acess(void);
bool	ACurses_GetDims_SerialTermHack(void);

// === GLOBALS ===
 int	giTerminal_Width = 80;
 int	giTerminal_Height = 25;

// === CODE ===
void ACurses_Init(void)
{
	if( ACurses_GetDims_Acess() ) {
	}
	else if( ACurses_GetDims_SerialTermHack() ) {
	}
	else {
		_SysDebug("note: assuming 80x25, can't get terminal dimensions");
		giTerminal_Width = 80;
		giTerminal_Height = 25;
	}
}

bool ACurses_GetDims_Acess(void)
{
	if( _SysIOCtl(1, DRV_IOCTL_TYPE, NULL) != DRV_TYPE_TERMINAL )
		return false;
	
	struct ptydims	dims;
	if( _SysIOCtl(1, PTY_IOCTL_GETDIMS, &dims) == -1 )
		return false;
	giTerminal_Width = dims.W;
	giTerminal_Height = dims.H;
	return true;
}

bool ACurses_GetDims_SerialTermHack(void)
{
	_SysDebug("ACurses_GetDims_SerialTermHack: Trying");
	// Set cursor to 1000,1000 , request cursor position, reset cursor to 0,0
	static const char req[] = "\033[1000;1000H\033[6n\033[H";
	fflush(stdin);
	_SysWrite(1, req, sizeof(req));
	int64_t timeout = 1000;
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	_SysSelect(1, &fds, NULL, NULL, &timeout, 0);
	if( !FD_ISSET(0, &fds) )
		return false;
	
	if( fgetc(stdin) != '\x1b' )
		return false;
	if( fgetc(stdin) != '[' )
		return false;
	if( fscanf(stdin, "%i;%i", &giTerminal_Width, &giTerminal_Height) != 2 )
		return false;
	if( fgetc(stdin) != 'R' )
		return false;
	
	return true;
}

void SetCursorPos(int Row, int Col)
{
	printf("\x1B[%i;%iH", Row, Col);
}

