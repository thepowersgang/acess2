/*
 * Acess2 Kernel - Mouse Mulitplexing Driver
 * - By John Hodge (thePowersGang)
 *
 * include/mouse_int.h
 * - Mouse mulitplexing interface header
 */
#ifndef _MOUSE__MOUSE_INT_H_
#define _MOUSE__MOUSE_INT_H_

#include <api_drv_joystick.h>

typedef struct sPointer	tPointer;

/*
 * \brief Input device type (doesn't hold much)
 */
struct sMouse
{
	tMouse	*Next;
	tPointer	*Pointer;
	 int	NumButtons;
	 int	NumAxies;
	Uint32	ButtonState;
	 int	LastAxisVal[];
};

#define MAX_BUTTONS	6
#define MAX_AXIES	4
#define MAX_FILESIZE	(sizeof(tJoystick_FileHeader) + MAX_AXIES*sizeof(tJoystick_Axis) + MAX_BUTTONS)

/**
 */
struct sPointer
{
	tPointer	*Next;

	tMouse	*FirstDev;

	// Node
	tVFS_Node	Node;

	// Data
	Uint8	FileData[MAX_FILESIZE];
	tJoystick_FileHeader	*FileHeader;
	tJoystick_Axis	*Axies;
	Uint8	*Buttons;

	// Limits for axis positions
	Uint16	AxisLimits[MAX_AXIES];
};

#endif

