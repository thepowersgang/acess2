/*
 * Acess2 GUI (AxWin) Version 3
 * - By John Hodge (thePowersGang)
 *
 * input.c
 * - Input abstraction
 */
#include <common.h>
#include <acess/sys.h>

// TODO: Move out to a common header
typedef struct
{
	int Num;
	int Value;
} tNumValue;
#define JOY_IOCTL_GETSETAXISLIMIT	6
#define JOY_IOCTL_GETSETAXISPOSITION	7

// === IMPORTS ===
// TODO: Move out
const char	*gsMouseDevice;
 int	giTerminalFD;
 int	giScreenWidth;
 int	giScreenHeight;

// === GLOBALS ===
 int	giMouseFD;

// === CODE ===
int Input_Init(void)
{
	tNumValue	num_value;

	// Open mouse for RW
	giMouseFD = open(gsMouseDevice, 3);

	// Set mouse limits
	// TODO: Update these if the screen resolution changes
	num_value.Num = 0;	num_value.Value = giScreenWidth;
	ioctl(giMouseFD, JOY_IOCTL_GETSETAXISLIMIT, &num_value);
	num_value.Value = giScreenWidth/2;
	ioctl(giMouseFD, JOY_IOCTL_GETSETAXISPOSITION, &num_value);

	num_value.Num = 1;	num_value.Value = giScreenHeight;
	ioctl(giMouseFD, JOY_IOCTL_GETSETAXISLIMIT, &num_value);
	num_value.Value = giScreenHeight/2;
	ioctl(giMouseFD, JOY_IOCTL_GETSETAXISPOSITION, &num_value);

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

		// Handle movement
		Video_SetCursorPos( mouseinfo.Axies[0].CursorPos, mouseinfo.Axies[1].CursorPos );
		

		// TODO: Handle button presses
	}
}
