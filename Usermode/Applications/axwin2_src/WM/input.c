/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"
#include <acess/sys.h>

// === CODE ===
int Input_Init(void)
{
	struct {
		 int	Num, Value;
	}	num_value;

	// Open mouse for RW
	giMouseFD = open(gsMouseDevice, 3);

	// Set mouse limits
	num_value.Num = 0;
	num_value.Value = giScreenWidth;
	ioctl(giMouseFD, 6, &num_value);

	num_value.Num = 1;
	num_value.Value = giScreenHeight;
	ioctl(giMouseFD, 6, &num_value);

	return 0;
}

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
		if( read(giTerminalFD, &codepoint, sizeof(codepoint)) != sizeof(codepoint) )
		{
			// oops, error
		}
		// TODO: pass on to message handler
		_SysDebug("Keypress 0x%x", codepoint);
	}

	if(FD_ISSET(giMouseFD, set))
	{
		struct sMouseInfo {
			uint16_t	NAxies;
			uint16_t	NButtons;
			struct sMouseAxis {
				 int16_t	MinValue;
				 int16_t	MaxValue;
				 int16_t	CurValue;
				uint16_t	CursorPos;
			}	Axies[2];
			uint8_t	Buttons[3];
		}	mouseinfo;
	
		seek(giMouseFD, 0, SEEK_SET);
		if( read(giMouseFD, &mouseinfo, sizeof(mouseinfo)) != sizeof(mouseinfo) )
		{
			// Not a 3 button mouse, oops
			return ;
		}

//		_SysDebug("sizeof(uint16_t) = %i, sizeof(int16_t) = %i",
//			sizeof(uint16_t), sizeof(int16_t));
//		_SysDebug("NAxies=%i,NButtons=%i", mouseinfo.NAxies, mouseinfo.NButtons);
//		_SysDebug("offsetof(Axies[0].MinValue) = %i", offsetof(struct sMouseInfo, Axies[0].MinValue));
//		_SysDebug("[0] = {MinValue=%i,MaxValue=%i,CurValue=%i}",
//			mouseinfo.Axies[0].MinValue, mouseinfo.Axies[0].MaxValue,
//			mouseinfo.Axies[0].CurValue
//			);
		// Handle movement
//		Video_SetCursorPos( mouseinfo.Axies[0], mouseinfo.Axies[1] );
		_SysDebug("Cursor to %i,%i", mouseinfo.Axies[0].CursorPos, mouseinfo.Axies[1].CursorPos);
	}
}
