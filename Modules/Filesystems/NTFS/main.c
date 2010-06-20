/*
 * Acess2 - NTFS Driver
 * By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess licence. See the
 * file COPYING for details.
 *
 * main.c - Driver core
 */
#define DEBUG	1
#define VERBOSE	0
#include "common.h"
#include <modules.h>

// === PROTOTYPES ===
 int	NTFS_Install(char **Arguments);
tVFS_Node	*NTFS_InitDevice(char *Devices, char **Options);
void	NTFS_Unmount(tVFS_Node *Node);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0A /*v0.1*/, FS_NTFS, NTFS_Install, NULL);
tVFS_Driver	gNTFS_FSInfo = {"ntfs", 0, NTFS_InitDevice, NTFS_Unmount, NULL};

tNTFS_Disk	gNTFS_Disks;

// === CODE ===
/**
 * \brief Installs the NTFS driver
 */
int NTFS_Install(char **Arguments)
{
	VFS_AddDriver( &gNTFS_FSInfo );
	return 1;
}

/**
 * \brief Mount a NTFS volume
 */
tVFS_Node *NTFS_InitDevice(char *Device, char **Options)
{
	char	*path, *host;
	tNTFS_Disk	*disk;
	
	disk = malloc( sizeof(tNTFS_Disk) );
	
	return &disk->RootNode;
}

/**
 * \brief Unmount an NTFS Disk
 */
void NTFS_Unmount(tVFS_Node *Node)
{
	
}
