/*
 * Acess2 Kernel - Mouse Mulitplexing Driver
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Mouse mulitplexing
 */
#define DEBUG	0
#define VERSION	VER2(0,1)
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <Input/Mouse/include/mouse.h>
#include "include/mouse_int.h"

// === PROTOTYPES ===
 int	Mouse_Install(char **Arguments);
 int	Mouse_Cleanup(void);
// - "User" side
char	*Mouse_Root_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*Mouse_Root_FindDir(tVFS_Node *Node, const char *Name);
 int	Mouse_Dev_IOCtl(tVFS_Node *Node, int ID, void *Data);
size_t	Mouse_Dev_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Data);
// - Device Side
tMouse	*Mouse_Register(const char *Name, int NumButtons, int NumAxies);
void	Mouse_RemoveInstance(tMouse *Handle);
void	Mouse_HandleEvent(tMouse *Handle, Uint32 ButtonState, int *AxisDeltas);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, Mouse, Mouse_Install, Mouse_Cleanup, NULL);
tVFS_NodeType	gMouse_RootNodeType = {
	.ReadDir = Mouse_Root_ReadDir,
	.FindDir = Mouse_Root_FindDir
};
tVFS_NodeType	gMouse_DevNodeType = {
	.IOCtl = Mouse_Dev_IOCtl,
	.Read = Mouse_Dev_Read
};
tDevFS_Driver	gMouse_DevInfo = {
	NULL, "Mouse",
	{ .Flags = VFS_FFLAG_DIRECTORY, .Type = &gMouse_RootNodeType, .Size = 1 }
};
tPointer	gMouse_Pointer;

// === CODE ===
/**
 * \brief Initialise the keyboard driver
 */
int Mouse_Install(char **Arguments)
{
	/// - Register with DevFS
	DevFS_AddDevice( &gMouse_DevInfo );

	gMouse_Pointer.Node.Type = &gMouse_DevNodeType;
	gMouse_Pointer.Node.ImplPtr = &gMouse_Pointer;
	gMouse_Pointer.FileHeader = (void*)gMouse_Pointer.FileData;
	gMouse_Pointer.Axies = (void*)( gMouse_Pointer.FileHeader + 1 );

	return 0;
}

/**
 * \brief Pre-unload cleanup function
 */
int Mouse_Cleanup(void)
{
	return 0;
}

// --- VFS Interface ---
char *Mouse_Root_ReadDir(tVFS_Node *Node, int Pos)
{
	if( Pos != 0 )	return NULL;
	return strdup("system");
}

tVFS_Node *Mouse_Root_FindDir(tVFS_Node *Node, const char *Name)
{
	if( strcmp(Name, "system") != 0 )	return NULL;
	return &gMouse_Pointer.Node;
}

static const char *csaIOCTL_NAMES[] = {DRV_IOCTLNAMES, DRV_JOY_IOCTLNAMES, NULL};
/**
 * \brief IOCtl handler for the mouse
 */
int Mouse_Dev_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tJoystick_NumValue	*numval = Data;
	tPointer	*ptr = Node->ImplPtr;
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_MOUSE, "Mouse", VERSION, csaIOCTL_NAMES);

	case JOY_IOCTL_GETSETAXISLIMIT:
		if( !numval || !CheckMem(numval, sizeof(*numval)) )
			return -1;
		LOG("GetSetAxisLimit %i = %i", numval->Num, numval->Value);
		if(numval->Num < 0 || numval->Num >= ptr->FileHeader->NAxies)
			return 0;
		if(numval->Value != -1)
			ptr->AxisLimits[numval->Num] = numval->Value;
		return ptr->AxisLimits[numval->Num];

	case JOY_IOCTL_GETSETAXISPOSITION:
		if( !numval || !CheckMem(numval, sizeof(*numval)) )
			return -1;
		if(numval->Num < 0 || numval->Num >= ptr->FileHeader->NAxies)
			return 0;
		if(numval->Value != -1)
			ptr->Axies[numval->Num].CursorPos = numval->Value;
		return ptr->Axies[numval->Num].CursorPos;
	}
	return -1;
}

/**
 * \brief Read from a device
 */
size_t Mouse_Dev_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Data)
{
	tPointer *ptr = Node->ImplPtr;
	 int	n_buttons = ptr->FileHeader->NButtons;
	 int	n_axies = ptr->FileHeader->NAxies;

	ENTER("pNode iLength pData", Node, Length, Data);

	// TODO: Locking (Acquire)

	Length = MIN(
		Length, 
		sizeof(tJoystick_FileHeader) + n_axies*sizeof(tJoystick_Axis) + n_buttons
		);

	// Mark as checked
	VFS_MarkAvaliable( Node, 0 );

	// Check if more than the header is requested
	if( Length > sizeof(tJoystick_FileHeader) )
	{
		// Clear axis values and button states
		for( int i = 0; i < n_axies; i ++ )
			ptr->Axies[i].CurValue = 0;
		for( int i = 0; i < n_buttons; i ++ )
			ptr->Buttons[i] = 0;

		// Rebuild from device list
		for( tMouse *dev = ptr->FirstDev; dev; dev = dev->Next )
		{
			for( int i = 0; i < n_axies; i ++ )
				ptr->Axies[i].CurValue += dev->LastAxisVal[i];
			for( int i = 0; i < n_buttons; i ++ )
			{
				if( dev->ButtonState & (1 << i) )
					ptr->Buttons[i] = 255;
			}
		}
	}

	memcpy( Data, ptr->FileData, Length );

	// TODO: Locking (Release)

	LEAVE('i', Length);
	return Length;
}

// --- Device Interface ---
/*
 * Register an input device
 * - See Input/Mouse/include/mouse.h
 */
tMouse *Mouse_Register(const char *Name, int NumButtons, int NumAxies)
{
	tPointer *target;
	tMouse   *ret;

	// TODO: Multiple pointers?
	target = &gMouse_Pointer;

	if( NumButtons > MAX_BUTTONS )
		NumButtons = MAX_BUTTONS;
	if( NumAxies > MAX_AXIES )
		NumAxies = MAX_AXIES;

	ret = malloc( sizeof(tMouse) + sizeof(ret->LastAxisVal[0])*NumAxies );
	if(!ret) {
		Log_Error("Mouse", "malloc() failed");
		return NULL;
	}
	ret->Next = NULL;
	ret->Pointer = target;
	ret->NumAxies = NumAxies;
	ret->NumButtons = NumButtons;
	ret->ButtonState = 0;
	memset(ret->LastAxisVal, 0, sizeof(ret->LastAxisVal[0])*NumAxies );

	// Add
	// TODO: Locking
	ret->Next = target->FirstDev;
	target->FirstDev = ret;
	if( ret->NumAxies > target->FileHeader->NAxies ) {
		// Clear new axis data
		memset(
			target->Axies + target->FileHeader->NAxies,
			0,
			(ret->NumAxies - target->FileHeader->NAxies)/sizeof(tJoystick_Axis)
			);
		target->FileHeader->NAxies = ret->NumAxies;
		target->Buttons = (void*)( target->Axies + ret->NumAxies );
	}
	if( ret->NumButtons > target->FileHeader->NButtons ) {
		// No need to move as buttons can expand out into space
		target->FileHeader->NButtons = ret->NumButtons;
	}
	// TODO: Locking

	return ret;
}

/*
 * Remove a mouse instance
 * - See Input/Mouse/include/mouse.h
 */
void Mouse_RemoveInstance(tMouse *Instance)
{
}

/*
 * Handle a mouse event (movement or button press/release)
 * - See Input/Mouse/include/mouse.h
 */
void Mouse_HandleEvent(tMouse *Handle, Uint32 ButtonState, int *AxisDeltas)
{
	tPointer *ptr;
	
	ENTER("pHandle xButtonState pAxisDeltas", Handle, ButtonState, AxisDeltas);

	ptr = Handle->Pointer;

	// Update device state
	Handle->ButtonState = ButtonState;
	memcpy(Handle->LastAxisVal, AxisDeltas, sizeof(*AxisDeltas)*Handle->NumAxies);

	// Update cursor position
	// TODO: Acceleration?
	for( int i = 0; i < Handle->NumAxies; i ++ )
	{
		ptr->Axies[i].CursorPos = MIN(MAX(0, ptr->Axies[i].CursorPos+AxisDeltas[i]), ptr->AxisLimits[i]);
		LOG("Axis %i: delta = %i, pos = %i", i, AxisDeltas[i], ptr->Axies[i].CursorPos);
	}

	VFS_MarkAvaliable( &ptr->Node, 1 );
	LEAVE('-');
}

