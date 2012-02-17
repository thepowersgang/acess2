/*
 * Acess2 Mouse Driver
 */
#define DEBUG	0
#include <acess.h>
#include <modules.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <api_drv_common.h>
#include <api_drv_joystick.h>
#include "common.h"

// == CONSTANTS ==
#define NUM_AXIES	2	// X+Y
#define NUM_BUTTONS	5	// Left, Right, Scroll Click, Scroll Up, Scroll Down

// == PROTOTYPES ==
// - Internal -
 int	PS2Mouse_Install(char **Arguments);
void	PS2Mouse_HandleInterrupt(Uint8 InputByte);
// - Filesystem -
size_t	PS2Mouse_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer);
 int	PS2Mouse_IOCtl(tVFS_Node *Node, int ID, void *Data);

// == GLOBALS ==
void	(*gpMouse_EnableFcn)(void);
// - Settings
 int	giMouse_Sensitivity = 1;
 int	giMouse_MaxX = 640, giMouse_MaxY = 480;
// - File Data
Uint8	gMouse_FileData[sizeof(tJoystick_FileHeader) + NUM_AXIES*sizeof(tJoystick_Axis) + NUM_BUTTONS];
tJoystick_FileHeader	*gMouse_FileHeader = (void *)gMouse_FileData;
tJoystick_Axis	*gMouse_Axies;
Uint8	*gMouse_Buttons;
tJoystick_Callback	gMouse_Callback;
 int	gMouse_CallbackArg;
 int	giMouse_AxisLimits[2];
// - Internal State
 int	giMouse_Cycle = 0;	// IRQ Position
Uint8	gaMouse_Bytes[4] = {0,0,0,0};
// - Driver definition
tVFS_NodeType	gMouse_NodeType = {
	.Read = PS2Mouse_Read,
	.IOCtl = PS2Mouse_IOCtl
};
tDevFS_Driver	gMouse_DriverStruct = {
	NULL, "PS2Mouse",
	{
	.NumACLs = 1, .ACLs = &gVFS_ACL_EveryoneRX,
	.Type = &gMouse_NodeType
	}
};

// == CODE ==
int PS2Mouse_Install(char **Arguments)
{
	

	// Set up variables
	gMouse_Axies = (void*)&gMouse_FileData[4];
	gMouse_Buttons = (void*)&gMouse_Axies[NUM_AXIES];

	gMouse_FileHeader->NAxies = 2;	gMouse_FileHeader->NButtons = 3;
	gMouse_Axies[0].MinValue = -10;	gMouse_Axies[0].MaxValue = 10;
	gMouse_Axies[1].MinValue = -10;	gMouse_Axies[1].MaxValue = 10;
	
	// Initialise Mouse Controller
	giMouse_Cycle = 0;	// Set Current Cycle position
	gpMouse_EnableFcn();
	
	DevFS_AddDevice(&gMouse_DriverStruct);
	
	return MODULE_ERR_OK;
}

/* Handle Mouse Interrupt
 */
void PS2Mouse_HandleInterrupt(Uint8 InputByte)
{
	Uint8	flags;
	 int	d[2], d_accel[2];
	 int	i;
	
	// Gather mouse data
	gaMouse_Bytes[giMouse_Cycle] = InputByte;
	LOG("gaMouse_Bytes[%i] = 0x%02x", gMouse_Axies[0].MaxValue);
	// - If bit 3 of the first byte is not set, it's not a valid packet?
	if(giMouse_Cycle == 0 && !(gaMouse_Bytes[0] & 0x08) )
		return ;
	giMouse_Cycle++;
	if(giMouse_Cycle < 3)
		return ;

	giMouse_Cycle = 0;

	// Actual Processing (once we have all bytes)	
	flags = gaMouse_Bytes[0];

	LOG("flags = 0x%x", flags);
	
	// Check for X/Y Overflow
	if(flags & 0xC0)	return;
		
	// Calculate dX and dY
	d[0] = gaMouse_Bytes[1];	if(flags & 0x10) d[0] = -(256-d[0]);	// x
	d[1] = gaMouse_Bytes[2];	if(flags & 0x20) d[1] = -(256-d[1]);	// y
	d[1] = -d[1];	// Y is negated
	LOG("RAW dx=%i, dy=%i\n", d[0], d[1]);
	// Apply scaling
	// TODO: Apply a form of curve to the mouse movement (dx*log(dx), dx^k?)
	// TODO: Independent sensitivities?
	// TODO: Disable acceleration via a flag?
	d_accel[0] = d[0]*giMouse_Sensitivity;
	d_accel[1] = d[1]*giMouse_Sensitivity;
	
	// Set Buttons (Primary)
	for( i = 0; i < 3; i ++ )
	{
		Uint8	newVal = (flags & (1 << i)) ? 0xFF : 0;
		if(newVal != gMouse_Buttons[i]) {
			if( gMouse_Callback )
				gMouse_Callback(gMouse_CallbackArg, 0, i, newVal - gMouse_Buttons[i]);
			gMouse_Buttons[i] = newVal;
		}
	}
	
	// Update X and Y Positions
	for( i = 0; i < 2; i ++ )
	{
		Sint16	newCursor = 0;
		if( giMouse_AxisLimits[i] )
			newCursor = MIN( MAX(0, gMouse_Axies[i].CursorPos + d_accel[i]), giMouse_AxisLimits[i] );;
		
		if( gMouse_Callback )
		{
			if(giMouse_AxisLimits[i] && gMouse_Axies[i].CursorPos != newCursor)
				gMouse_Callback(gMouse_CallbackArg, 1, i, newCursor - gMouse_Axies[i].CursorPos);
			if(!giMouse_AxisLimits[i] && gMouse_Axies[i].CurValue != d_accel[i])
				gMouse_Callback(gMouse_CallbackArg, 1, i, d_accel[i] - gMouse_Axies[i].CurValue);
		}
		
		gMouse_Axies[i].CurValue = d_accel[i];
		gMouse_Axies[i].CursorPos = newCursor;
	}

//	Log("Mouse at %ix%i", gMouse_Axies[0].CursorPos, gMouse_Axies[1].CursorPos);
		
	VFS_MarkAvaliable(&gMouse_DriverStruct.RootNode, 1);
}

/* Read mouse state (coordinates)
 */
size_t PS2Mouse_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer)
{
	if(Offset > sizeof(gMouse_FileData))	return 0;
	if(Length > sizeof(gMouse_FileData))	Length = sizeof(gMouse_FileData);
	if(Offset + Length > sizeof(gMouse_FileData))	Length = sizeof(gMouse_FileData) - Offset;

	memcpy(Buffer, &gMouse_FileData[Offset], Length);
	
	VFS_MarkAvaliable(Node, 0);
	return Length;
}

static const char *csaIOCtls[] = {DRV_IOCTLNAMES, DRV_JOY_IOCTLNAMES, NULL};
/* Handle messages to the device
 */
int PS2Mouse_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tJoystick_NumValue	*info = Data;

	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_JOYSTICK, "PS2Mouse", 0x100, csaIOCtls);

	case JOY_IOCTL_SETCALLBACK:	// TODO: Implement
		return -1;
	
	case JOY_IOCTL_SETCALLBACKARG:	// TODO: Implement
		return -1;
	
	case JOY_IOCTL_GETSETAXISLIMIT:
		if(!info)	return 0;
		if(info->Num < 0 || info->Num >= 2)	return 0;
		if(info->Value != -1)
			giMouse_AxisLimits[info->Num] = info->Value;
		return giMouse_AxisLimits[info->Num];
	
	case JOY_IOCTL_GETSETAXISPOSITION:
		if(!info)	return 0;
		if(info->Num < 0 || info->Num >= 2)	return 0;
		if(info->Value != -1)
			gMouse_Axies[info->Num].CursorPos = info->Value;
		return gMouse_Axies[info->Num].CursorPos;

	case JOY_IOCTL_GETSETAXISFLAGS:
		return -1;
	
	case JOY_IOCTL_GETSETBUTTONFLAGS:
		return -1;

	default:
		return 0;
	}
}

