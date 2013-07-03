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
#include "include/mouse_int.h"

// === PROTOTYPES ===
 int	Mouse_Install(char **Arguments);
 int	Mouse_Cleanup(void);
// - "User" side
 int	Mouse_Root_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX]);
tVFS_Node	*Mouse_Root_FindDir(tVFS_Node *Node, const char *Name, Uint Flags);
 int	Mouse_Dev_IOCtl(tVFS_Node *Node, int ID, void *Data);
size_t	Mouse_Dev_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Data, Uint Flags);
// - Device Side
void	Mouse_HandleEvent(Uint32 ButtonState, int *AxisDeltas, int *AxisValues);

// === GLOBALS ===
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
	gMouse_Pointer.Buttons = (void*)( gMouse_Pointer.Axies + MAX_AXIES );
	gMouse_Pointer.FileHeader->NAxies = MAX_AXIES;
	gMouse_Pointer.FileHeader->NButtons = MAX_BUTTONS;

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
int Mouse_Root_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX])
{
	if( Pos != 0 )
		return -EINVAL;
	strcpy(Dest, "system");
	return 0;
}

tVFS_Node *Mouse_Root_FindDir(tVFS_Node *Node, const char *Name, Uint Flags)
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
size_t Mouse_Dev_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Data, Uint Flags)
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

	memcpy( Data, ptr->FileData, Length );

	// TODO: Locking (Release)

	LEAVE('i', Length);
	return Length;
}

// --- Device Interface ---
/*
 * Handle a mouse event (movement or button press/release)
 * - See Input/Mouse/include/mouse.h
 */
void Mouse_HandleEvent(Uint32 ButtonState, int *AxisDeltas, int *AxisValues)
{
	tPointer *ptr = &gMouse_Pointer;
	
	ENTER("pHandle xButtonState pAxisDeltas", Handle, ButtonState, AxisDeltas);

	// Update cursor position
	for( int i = 0; i < 2; i ++ )
	{
		ptr->Axies[i].CursorPos = AxisValues[i];
		ptr->Axies[i].CurValue = AxisDeltas ? AxisDeltas[i] : 0;
	}
	for( int i = 0; i < 5; i ++ )
		ptr->Buttons[i] = ButtonState & (1 << i) ? 255 : 0;

	VFS_MarkAvaliable( &ptr->Node, 1 );
	LEAVE('-');
}

