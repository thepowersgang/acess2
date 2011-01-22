/*
 * Acess2 Native Kernel
 * 
 * Keyboard Driver
 */
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <tpl_drv_common.h>
#include <tpl_drv_keyboard.h>
#include "ui.h"

// === PROTOTYPES ===
 int	NativeKeyboard_Install(char **Arguments);
 int	NativeKeyboard_IOCtl(tVFS_Node *Node, int Id, void *Data);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0100, NativeKeyboard, NativeKeyboard_Install, NULL, NULL);
tDevFS_Driver	gKB_DevInfo = {
	NULL, "NativeKeyboard",
	{
	.NumACLs = 0,
	.Size = 0,
	.IOCtl = NativeKeyboard_IOCtl
	}
};

// === CODE ===
/**
 * \brief Install the keyboard driver
 */
int NativeKeyboard_Install(char **Arguments)
{
	DevFS_AddDevice( &gKB_DevInfo );
	return MODULE_ERR_OK;
}

static const char * const csaIOCTL_NAMES[] = {
	DRV_IOCTLNAMES,
	DRV_KEYBAORD_IOCTLNAMES,
	NULL
};

/**
 * \fn int KB_IOCtl(tVFS_Node *Node, int Id, void *Data)
 * \brief Calls an IOCtl Command
 */
int NativeKeyboard_IOCtl(tVFS_Node *Node, int Id, void *Data)
{
	switch(Id)
	{
	BASE_IOCTLS(DRV_TYPE_KEYBOARD, "NativeKeyboard", 0x10000, csaIOCTL_NAMES);

	// Sets the Keyboard Callback
	case KB_IOCTL_SETCALLBACK:
		// Sanity Check
		if(Threads_GetUID() != 0)
			return 0;
		// Can only be set once
		if(gUI_KeyboardCallback != NULL)	return 0;
		// Set Callback
		gUI_KeyboardCallback = Data;
		return 1;

	default:
		return 0;
	}
}
