/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"
#include <acess/sys.h>

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
		uint32_t	codepoint;
		if( read(giTerminalFD, sizeof(codepoint), &codepoint) != sizeof(codepoint) )
		{
			// oops, error
		}
		// TODO: pass on to message handler
	}

	if(FD_ISSET(giMouseFD, set))
	{
		struct sMouseInfo {
			uint16_t	NAxies, NButtons;
			struct sMouseAxis {
				 int16_t	MinValue, MaxValue;
				 int16_t	CurValue;
				uint16_t	CursorPos;
			}	Axies[2];
			uint8_t	Buttons[3];
		}	mouseinfo;
		
		if( read(giMouseFD, sizeof(mouseinfo), &mouseinfo) != sizeof(mouseinfo) )
		{
			// Not a 3 button mouse, oops
			return ;
		}
		
		// Handle movement
//		Video_SetCursorPos( mouseinfo.Axies[0], mouseinfo.Axies[1] );
	}
}
