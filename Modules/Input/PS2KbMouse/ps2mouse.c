/*
 * Acess2 Mouse Driver
 */
#define DEBUG	0
#include <acess.h>
#include <modules.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <tpl_drv_common.h>
#include <tpl_drv_joystick.h>

static inline int MIN(int a, int b) { return (a < b) ? a : b; }
static inline int MAX(int a, int b) { return (a > b) ? a : b; }

// == CONSTANTS ==
#define NUM_AXIES	2	// X+Y
#define NUM_BUTTONS	5	// Left, Right, Scroll Click, Scroll Up, Scroll Down
#define PS2_IO_PORT	0x60

// == PROTOTYPES ==
// - Internal -
 int	PS2Mouse_Install(char **Arguments);
void	PS2Mouse_IRQ(int Num);
static void	mouseSendCommand(Uint8 cmd);
void	PS2Mouse_Enable();
// - Filesystem -
Uint64	PS2Mouse_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
int	PS2Mouse_IOCtl(tVFS_Node *Node, int ID, void *Data);

// == GLOBALS ==
// - Settings
 int	giMouse_Sensitivity = 1;
 int	giMouse_MaxX = 640, giMouse_MaxY = 480;
// - File Data
Uint8	gMouse_FileData[sizeof(tJoystick_FileHeader) + NUM_AXIES*sizeof(tJoystick_Axis) + NUM_BUTTONS];
tJoystick_FileHeader	*gMouse_FileHeader = (void *)gMouse_FileData;
tJoystick_Axis	*gMouse_Axies;
Uint8	*gMouse_Buttons;
// - Internal State
 int	giMouse_Cycle = 0;	// IRQ Position
Uint8	gaMouse_Bytes[4] = {0,0,0,0};
// - Driver definition
tDevFS_Driver	gMouse_DriverStruct = {
	NULL, "PS2Mouse",
	{
	.NumACLs = 1, .ACLs = &gVFS_ACL_EveryoneRX,
	.Read = PS2Mouse_Read, .IOCtl = PS2Mouse_IOCtl
	}
};

// == CODE ==
int PS2Mouse_Install(char **Arguments)
{
	// Set up variables
	gMouse_Axies = (void*)&gMouse_FileData[1];
	gMouse_Buttons = (void*)&gMouse_Axies[NUM_AXIES];
	
	// Initialise Mouse Controller
	IRQ_AddHandler(12, PS2Mouse_IRQ);	// Set IRQ
	giMouse_Cycle = 0;	// Set Current Cycle position
	PS2Mouse_Enable();		// Enable the mouse
	
	DevFS_AddDevice(&gMouse_DriverStruct);
	
	return MODULE_ERR_OK;
}

/* Handle Mouse Interrupt
 */
void PS2Mouse_IRQ(int Num)
{
	Uint8	flags;
	 int	dx, dy;
	 int	dx_accel, dy_accel;

	
	// Gather mouse data
	gaMouse_Bytes[giMouse_Cycle] = inb(0x60);
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
	dx = gaMouse_Bytes[1];	if(flags & 0x10) dx = -(256-dx);
	dy = gaMouse_Bytes[2];	if(flags & 0x20) dy = -(256-dy);
	dy = -dy;	// Y is negated
	LOG("RAW dx=%i, dy=%i\n", dx, dy);
	// Apply scaling
	// TODO: Apply a form of curve to the mouse movement (dx*log(dx), dx^k?)
	// TODO: Independent sensitivities?
	// TODO: Disable acceleration via a flag?
	dx_accel = dx*giMouse_Sensitivity;
	dy_accel = dy*giMouse_Sensitivity;
	
	// Set Buttons (Primary)
	gMouse_Buttons[0] = (flags & 1) ? 0xFF : 0;
	gMouse_Buttons[1] = (flags & 2) ? 0xFF : 0;
	gMouse_Buttons[2] = (flags & 4) ? 0xFF : 0;
	
	// Update X and Y Positions
	gMouse_Axies[0].CurValue = MIN( MAX(gMouse_Axies[0].MinValue, gMouse_Axies[0].CurValue + dx_accel), gMouse_Axies[0].MaxValue );
	gMouse_Axies[1].CurValue = MIN( MAX(gMouse_Axies[1].MinValue, gMouse_Axies[1].CurValue + dy_accel), gMouse_Axies[1].MaxValue );
}

/* Read mouse state (coordinates)
 */
Uint64 PS2Mouse_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	if(Offset > sizeof(gMouse_FileData))	return 0;
	if(Length > sizeof(gMouse_FileData))	Length = sizeof(gMouse_FileData);
	if(Offset + Length > sizeof(gMouse_FileData))	Length = sizeof(gMouse_FileData) - Offset;

	memcpy(Buffer, &gMouse_FileData[Offset], Length);
		
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
			gMouse_Axies[info->Num].MaxValue = info->Value;
		return gMouse_Axies[info->Num].MaxValue;
	
	case JOY_IOCTL_GETSETAXISPOSITION:
		if(!info)	return 0;
		if(info->Num < 0 || info->Num >= 2)	return 0;
		if(info->Value != -1)
			gMouse_Axies[info->Num].CurValue = info->Value;
		return gMouse_Axies[info->Num].CurValue;

	case JOY_IOCTL_GETSETAXISFLAGS:
		return -1;
	
	case JOY_IOCTL_GETSETBUTTONFLAGS:
		return -1;

	default:
		return 0;
	}
}

//== Internal Functions ==
static inline void mouseOut64(Uint8 data)
{
	int timeout=100000;
	while( timeout-- && inb(0x64) & 2 );	// Wait for Flag to clear
	outb(0x64, data);	// Send Command
}
static inline void mouseOut60(Uint8 data)
{
	int timeout=100000;
	while( timeout-- && inb(0x64) & 2 );	// Wait for Flag to clear
	outb(0x60, data);	// Send Command
}
static inline Uint8 mouseIn60(void)
{
	int timeout=100000;
	while( timeout-- && (inb(0x64) & 1) == 0);	// Wait for Flag to set
	return inb(0x60);
}
static inline void mouseSendCommand(Uint8 cmd)
{
	mouseOut64(0xD4);
	mouseOut60(cmd);
}

void PS2Mouse_Enable(void)
{
	Uint8	status;
	Log_Log("PS2Mouse", "Enabling Mouse...");
	
	// Enable AUX PS/2
	mouseOut64(0xA8);
	
	// Enable AUX PS/2 (Compaq Status Byte)
	mouseOut64(0x20);	// Send Command
	status = mouseIn60();	// Get Status
	status &= ~0x20;	// Clear "Disable Mouse Clock"
	status |= 0x02; 	// Set IRQ12 Enable
	mouseOut64(0x60);	// Send Command
	mouseOut60(status);	// Set Status
	
	//mouseSendCommand(0xF6);	// Set Default Settings
	mouseSendCommand(0xF4);	// Enable Packets
}
