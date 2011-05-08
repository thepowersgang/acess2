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

// == CONSTANTS ==
#define PS2_IO_PORT	0x60
#define MOUSE_BUFFER_SIZE	(sizeof(t_mouse_fsdata))
#define MOUSE_SENS_BASE	5

// == PROTOTYPES ==
// - Internal -
 int	PS2Mouse_Install(char **Arguments);
void	PS2Mouse_IRQ(int Num);
static void	mouseSendCommand(Uint8 cmd);
static void	enableMouse();
// - Filesystem -
static Uint64	PS2Mouse_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
static int	PS2Mouse_IOCtl(tVFS_Node *Node, int ID, void *Data);

typedef struct {
	int x, y, buttons, unk;
}	t_mouse_fsdata;

// == GLOBALS ==
 int	giMouse_Sensitivity = 1;
t_mouse_fsdata	gMouse_Data = {0, 0, 0, 0};
 int giMouse_MaxX = 640, giMouse_MaxY = 480;
 int	giMouse_Cycle = 0;	// IRQ Position
Uint8	gaMouse_Bytes[4] = {0,0,0,0};
tDevFS_Driver	gMouse_DriverStruct = {
	NULL, "ps2mouse",
	{
	.NumACLs = 1, .ACLs = &gVFS_ACL_EveryoneRX,
	.Read = PS2Mouse_Read, .IOCtl = PS2Mouse_IOCtl
	}
};

// == CODE ==
int Mouse_Install(char **Arguments)
{
	// Initialise Mouse Controller
	IRQ_AddHandler(12, PS2Mouse_IRQ);	// Set IRQ
	giMouse_Cycle = 0;	// Set Current Cycle position
	enableMouse();		// Enable the mouse
	
	DevFS_AddDevice(&gMouse_DriverStruct);
	
	return MODULE_ERR_OK;
}

/* Handle Mouse Interrupt
 */
void PS2Mouse_IRQ(int Num)
{
	Uint8	flags;
	int	dx, dy;
	//int	log2x, log2y;
	
	gaMouse_Bytes[giMouse_Cycle] = inb(0x60);
	if(giMouse_Cycle == 0 && !(gaMouse_Bytes[giMouse_Cycle] & 0x8))
		return;
	giMouse_Cycle++;
	if(giMouse_Cycle == 3)
		giMouse_Cycle = 0;
	else
		return;
	
	if(giMouse_Cycle > 0)
		return;
	
	// Read Flags
	flags = gaMouse_Bytes[0];
	if(flags & 0xC0)	// X/Y Overflow
		return;
		
	#if DEBUG
	//LogF(" PS2Mouse_Irq: flags = 0x%x\n", flags);
	#endif
	
	// Calculate dX and dY
	dx = (int) ( gaMouse_Bytes[1] | (flags & 0x10 ? ~0x00FF : 0) );
	dy = (int) ( gaMouse_Bytes[2] | (flags & 0x20 ? ~0x00FF : 0) );
	dy = -dy;	// Y is negated
	LOG("RAW dx=%i, dy=%i\n", dx, dy);
	dx = dx*MOUSE_SENS_BASE/giMouse_Sensitivity;
	dy = dy*MOUSE_SENS_BASE/giMouse_Sensitivity;
	
	//__asm__ __volatile__ ("bsr %%eax, %%ecx" : "=c" (log2x) : "a" ((Uint)gaMouse_Bytes[1]));
	//__asm__ __volatile__ ("bsr %%eax, %%ecx" : "=c" (log2y) : "a" ((Uint)gaMouse_Bytes[2]));
	//LogF(" PS2Mouse_Irq: dx=%i, log2x = %i\n", dx, log2x);
	//LogF(" PS2Mouse_Irq: dy=%i, log2y = %i\n", dy, log2y);
	//dx *= log2x;
	//dy *= log2y;
	
	// Set Buttons
	gMouse_Data.buttons = flags & 0x7;	//Buttons (3 only)
	
	// Update X and Y Positions
	gMouse_Data.x += (int)dx;
	gMouse_Data.y += (int)dy;
	
	// Constrain Positions
	if(gMouse_Data.x < 0)	gMouse_Data.x = 0;
	if(gMouse_Data.y < 0)	gMouse_Data.y = 0;
	if(gMouse_Data.x >= giMouse_MaxX)	gMouse_Data.x = giMouse_MaxX-1;
	if(gMouse_Data.y >= giMouse_MaxY)	gMouse_Data.y = giMouse_MaxY-1;	
}

/* Read mouse state (coordinates)
 */
static Uint64 PS2Mouse_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	if(Length < MOUSE_BUFFER_SIZE)
		return -1;

	memcpy(Buffer, &gMouse_Data, MOUSE_BUFFER_SIZE);
		
	return MOUSE_BUFFER_SIZE;
}

static const char *csaIOCtls[] = {DRV_IOCTLNAMES, NULL};
/* Handle messages to the device
 */
static int PS2Mouse_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	struct {
		 int	Num;
		 int	Value;
	}	*info = Data;
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_JOYSTICK, "PS2Mouse", 0x100, csaIOCtls);
	
	case JOY_IOCTL_GETSETAXISLIMIT:
		if(!info)	return 0;
		switch(info->Num)
		{
		case 0:	// X
			if(info->Value != -1)
				giMouse_MaxX = info->Value;
			return giMouse_MaxX;
		case 1:	// Y
			if(info->Value != -1)
				giMouse_MaxY = info->Value;
			return giMouse_MaxY;
		}
		return 0;
		
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
static inline Uint8 mouseIn60()
{
	int timeout=100000;
	while( timeout-- && (inb(0x64) & 1) == 0);	// Wait for Flag to set
	return inb(0x60);
}
static void mouseSendCommand(Uint8 cmd)
{
	mouseOut64(0xD4);
	mouseOut60(cmd);
}

static void enableMouse()
{
	Uint8	status;
	Log_Log("PS2 Mouse", "Enabling Mouse...");
	
	// Enable AUX PS/2
	mouseOut64(0xA8);
	
	// Enable AUX PS/2 (Compaq Status Byte)
	mouseOut64(0x20);	// Send Command
	status = mouseIn60();	// Get Status
	status &= 0xDF;	status |= 0x02;	// Alter Flags (Set IRQ12 (2) and Clear Disable Mouse Clock (20))
	mouseOut64(0x60);	// Send Command
	mouseOut60(status);	// Set Status
	
	//mouseSendCommand(0xF6);	// Set Default Settings
	mouseSendCommand(0xF4);	// Enable Packets
}
