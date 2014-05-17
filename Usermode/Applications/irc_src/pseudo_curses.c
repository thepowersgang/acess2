/*
 */
#include "pseudo_curses.h"
#include <acess/sys.h>
#include <acess/devices/pty.h>
#include <stdio.h>

 int	giTerminal_Width = 80;
 int	giTerminal_Height = 25;

void ACurses_Init(void)
{
	if( _SysIOCtl(1, DRV_IOCTL_TYPE, NULL) != DRV_TYPE_TERMINAL ) {
		_SysDebug("note: assuming 80x25, can't get terminal dimensions");
		giTerminal_Width = 80;
		giTerminal_Height = 25;
	}
	else {
		struct ptydims	dims;
		_SysIOCtl(1, PTY_IOCTL_GETDIMS, &dims);
		giTerminal_Width = dims.W;
		giTerminal_Height = dims.H;
	}
}

void SetCursorPos(int Row, int Col)
{
	printf("\x1B[%i;%iH", Row, Col);
}

