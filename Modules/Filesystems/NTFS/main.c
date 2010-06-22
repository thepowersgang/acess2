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
	tNTFS_Disk	*disk;
	tNTFS_BootSector	bs;
	
	disk = malloc( sizeof(tNTFS_Disk) );
	
	disk->FD = VFS_Open(Device, VFS_OPENFLAG_READ);
	if(!disk->FD) {
		free(disk);
		return NULL;
	}
	
	VFS_ReadAt(disk->FD, 0, 512, &bs);
	
	disk->ClusterSize = bs.BytesPerSector * bs.SectorsPerCluster;
	Log_Debug("NTFS", "Cluster Size = %i KiB", disk->ClusterSize/1024);
	disk->MFTBase = bs.MFTStart;
	Log_Debug("NTFS", "MFT Base = %i", disk->MFTBase);
	
	disk->RootNode.Inode = 5;	// MFT Ent #5 is '.'
	disk->RootNode.ImplPtr = disk;
	
	disk->RootNode.UID = 0;
	disk->RootNode.GID = 0;
	
	disk->RootNode.NumACLs = 1;
	disk->RootNode.ACLs = &gVFS_ACL_EveryoneRX;
	
	disk->RootNode.ReadDir = NTFS_ReadDir;
	disk->RootNode.FindDir = NTFS_FindDir;
	disk->RootNode.MkNod = NULL;
	disk->RootNode.Link = NULL;
	disk->RootNode.Relink = NULL;
	disk->RootNode.Close = NULL;
	
	return &disk->RootNode;
}

/**
 * \brief Unmount an NTFS Disk
 */
void NTFS_Unmount(tVFS_Node *Node)
{
	
}
