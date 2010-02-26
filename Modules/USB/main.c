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

// === IMPORTS ===
 int	UHCI_Initialise();

// === PROTOTYPES ===
 int	USB_Install(char **Arguments);
void	USB_Cleanup();
char	*USB_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*USB_FindDir(tVFS_Node *Node, char *Name);
 int	USB_IOCtl(tVFS_Node *Node, int Id, void *Data);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, USB, USB_Install, NULL, NULL);
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
tUSBDevice	*gUSB_Devices = NULL;
tUSBHost	*gUSB_Hosts = NULL;

// === CODE ===
/**
 * \fn int ModuleLoad()
 * \brief Called once module is loaded
 */
int USB_Install(char **Arguments)
{
	UHCI_Initialise();
	Warning("[USB  ] Not Complete - Devel Only");
	return 1;
}

/**
 * \fn void USB_Cleanup()
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
 * \fn tVFS_Node *USB_FindDir(tVFS_Node *Node, char *Name)
 * \brief Locate an entry in the USB root
 */
tVFS_Node *USB_FindDir(tVFS_Node *Node, char *Name)
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
