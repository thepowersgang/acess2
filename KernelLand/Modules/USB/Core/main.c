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
extern void	USB_PollThread(void *unused);
extern void	USB_AsyncThread(void *Unused);

// === PROTOTYPES ===
 int	USB_Install(char **Arguments);
void	USB_Cleanup(void);
char	*USB_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*USB_FindDir(tVFS_Node *Node, const char *Name);
 int	USB_IOCtl(tVFS_Node *Node, int Id, void *Data);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, USB_Core, USB_Install, NULL, NULL);
tVFS_NodeType	gUSB_RootNodeType = {
	.ReadDir = USB_ReadDir,
	.FindDir = USB_FindDir,
	.IOCtl = USB_IOCtl
};
tDevFS_Driver	gUSB_DrvInfo = {
	NULL, "usb", {
		.NumACLs = 1,
		.ACLs = &gVFS_ACL_EveryoneRX,
		.Flags = VFS_FFLAG_DIRECTORY,
		.Type = &gUSB_RootNodeType
	}
};

// === CODE ===
/**
 * \brief Called once module is loaded
 */
int USB_Install(char **Arguments)
{
	Proc_SpawnWorker(USB_PollThread, NULL);
	Proc_SpawnWorker(USB_AsyncThread, NULL);
	
	return MODULE_ERR_OK;
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
