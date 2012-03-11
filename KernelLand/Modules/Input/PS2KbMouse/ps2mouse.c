/*
 * Acess2 Mouse Driver
 */
#define DEBUG	0
#include <acess.h>
#include <modules.h>
#include <vfs.h>
#include <Input/Mouse/include/mouse.h>
#include "common.h"

// == CONSTANTS ==
#define NUM_AXIES	2	// X+Y
#define NUM_BUTTONS	5	// Left, Right, Scroll Click, Scroll Up, Scroll Down

// == PROTOTYPES ==
// - Internal -
 int	PS2Mouse_Install(char **Arguments);
void	PS2Mouse_HandleInterrupt(Uint8 InputByte);

// == GLOBALS ==
void	(*gpPS2Mouse_EnableFcn)(void);
// - Settings
 int	giPS2Mouse_Sensitivity = 1;
// - Internal State
 int	giPS2Mouse_Cycle = 0;	// IRQ Position
Uint8	gaPS2Mouse_Bytes[4] = {0,0,0,0};
tMouse	*gpPS2Mouse_Handle;

// == CODE ==
int PS2Mouse_Install(char **Arguments)
{
	gpPS2Mouse_Handle = Mouse_Register("PS2Mouse", NUM_AXIES, NUM_BUTTONS);

	// Initialise Mouse Controller
	giPS2Mouse_Cycle = 0;	// Set Current Cycle position
	gpPS2Mouse_EnableFcn();

	return MODULE_ERR_OK;
}

/* Handle Mouse Interrupt
 */
void PS2Mouse_HandleInterrupt(Uint8 InputByte)
{
	Uint8	flags;
	 int	d[2], d_accel[2];
	
	// Gather mouse data
	gaPS2Mouse_Bytes[giPS2Mouse_Cycle] = InputByte;
	// - If bit 3 of the first byte is not set, it's not a valid packet?
	if(giPS2Mouse_Cycle == 0 && !(gaPS2Mouse_Bytes[0] & 0x08) )
		return ;

	// 3 bytes per packet
	giPS2Mouse_Cycle++;
	if(giPS2Mouse_Cycle < 3)
		return ;
	giPS2Mouse_Cycle = 0;

	// Actual Processing (once we have all bytes)	
	flags = gaPS2Mouse_Bytes[0];

	LOG("flags = 0x%x", flags);
	
	// Check for X/Y Overflow
	if(flags & 0xC0)	return;
		
	// Calculate dX and dY
	d[0] = gaPS2Mouse_Bytes[1];	if(flags & 0x10) d[0] = -(256-d[0]);	// x
	d[1] = gaPS2Mouse_Bytes[2];	if(flags & 0x20) d[1] = -(256-d[1]);	// y
	d[1] = -d[1];	// Y is negated
	LOG("RAW dx=%i, dy=%i\n", d[0], d[1]);
	// Apply scaling
	// TODO: Apply a form of curve to the mouse movement (dx*log(dx), dx^k?)
	// TODO: Independent sensitivities?
	// TODO: Disable acceleration via a flag?
	d_accel[0] = d[0]*giPS2Mouse_Sensitivity;
	d_accel[1] = d[1]*giPS2Mouse_Sensitivity;
	
	// TODO: Scroll wheel?	
	Mouse_HandleEvent(gpPS2Mouse_Handle, (flags & 7), d_accel);
}

