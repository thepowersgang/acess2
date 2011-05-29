/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"

// === CODE ===
void Input_FillSelect(int *nfds, fd_set *set)
{
	if(*nfds < giTerminalFD)	*nfds = giTerminalFD;
	if(*nfds < giMouseFD)	*nfds = giMouseFD;
	FD_SET(giTerminalFD, set);
	FD_SET(giMouseFD, set);
}

void Input_HandleSelect(fd_set *set)
{
	if(FD_ISSET(giTerminalFD, set))
	{
		
	}
}
