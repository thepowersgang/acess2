
#ifndef _ACESSNATIVE__MOUSE_INT_H_
#define _ACESSNATIVE__MOUSE_INT_H_

#include <api_drv_joystick.h>

typedef struct sPointer	tPointer;

#define MAX_BUTTONS	5
#define MAX_AXIES	2
#define MAX_FILESIZE	(sizeof(tJoystick_FileHeader) + MAX_AXIES*sizeof(tJoystick_Axis) + MAX_BUTTONS)

/**
 */
struct sPointer
{
	tPointer	*Next;

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

extern void	Mouse_HandleEvent(Uint32 ButtonState, int *AxisDeltas, int *AxisValues);

#endif

