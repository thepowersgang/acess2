/*
 * Acess2
 * USB Stack
 */
#define VERSION	( (0<<8)| 5 )
#define DEBUG	1
#include <acess.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <modules.h>
#include "usb.h"

// === PROTOTYPES ===
 int	USB_Install(char **Arguments);
void	USB_Cleanup(void);
char	*USB_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*USB_FindDir(tVFS_Node *Node, const char *Name);
 int	USB_IOCtl(tVFS_Node *Node, int Id, void *Data);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, USB_Core, USB_Install, NULL, NULL);
tDevFS_Driver	gUSB_DrvInfo = {
	NULL, "usb", {
		.NumACLs = 1,
		.ACLs = &gVFS_ACL_EveryoneRX,
		.Flags = VFS_FFLAG_DIRECTORY,
		.ReadDir = USB_ReadDir,
		.FindDir = USB_FindDir,
		.IOCtl = USB_IOCtl
	}
};
tUSBHost	*gUSB_Hosts = NULL;
tUSBDevice	*gUSB_InterruptDevs = NULL;
tUSBDevice	*gUSB_InterruptLast = NULL;

// === CODE ===
/**
 * \brief Called once module is loaded
 */
int USB_Install(char **Arguments)
{
	Log_Warning("USB", "Not Complete - Devel Only");
	return MODULE_ERR_OK;
}

/**
 * \brief USB polling thread
 */
int USB_PollThread(void *unused)
{
	for(;;)
	{
		for( tUSBDevice *dev = gUSB_InterruptDevs; dev; dev = dev->Next )
		{
//			hub->CheckPorts(hub, hub->Device);
		}
		// TODO: Fine tune
		Time_Delay(250);
	}
}

/**
 * \brief Called just before module is unloaded
 */
void USB_Cleanup()
{
}

/**
 * \fn char *USB_ReadDir(tVFS_Node *Node, int Pos)
 * \brief Read from the USB root
 */
char *USB_ReadDir(tVFS_Node *Node, int Pos)
{
	return NULL;
}

/**
 * \fn tVFS_Node *USB_FindDir(tVFS_Node *Node, const char *Name)
 * \brief Locate an entry in the USB root
 */
tVFS_Node *USB_FindDir(tVFS_Node *Node, const char *Name)
{
	return NULL;
}

/**
 * \brief Handles IOCtl Calls to the USB driver
 */
int USB_IOCtl(tVFS_Node *Node, int Id, void *Data)
{
	return 0;
}
