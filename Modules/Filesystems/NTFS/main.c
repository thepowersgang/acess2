/*
 * Acess2 - NTFS Driver
 * By John Hodge (thePowersGang)
 *
 * main.c - Driver core
 */
#define DEBUG	1
#define VERBOSE	0
#include <acess.h>
#include <vfs.h>
#include "common.h"
#include <modules.h>

// === IMPORTS ===
extern char	*NTFS_ReadDir(tVFS_Node *Node, int Pos);
extern tVFS_Node	*NTFS_FindDir(tVFS_Node *Node, char *Name);

// === PROTOTYPES ===
 int	NTFS_Install(char **Arguments);
tVFS_Node	*NTFS_InitDevice(char *Devices, char **Options);
void	NTFS_Unmount(tVFS_Node *Node);
void	NTFS_DumpEntry(tNTFS_Disk *Disk, Uint32 Entry);

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
	return 0;
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
	
	Log_Debug("FS_NTFS", "&bs = %p", &bs);
	VFS_ReadAt(disk->FD, 0, 512, &bs);
	
	Log_Debug("FS_NTFS", "Jump = %02x%02x%02x",
		bs.Jump[0],
		bs.Jump[1],
		bs.Jump[2]);
	Log_Debug("FS_NTFS", "SystemID = %02x%02x%02x%02x%02x%02x%02x%02x (%8C)",
		bs.SystemID[0], bs.SystemID[1], bs.SystemID[2], bs.SystemID[3],
		bs.SystemID[4], bs.SystemID[5], bs.SystemID[6], bs.SystemID[7],
		bs.SystemID
		);
	Log_Debug("FS_NTFS", "BytesPerSector = %i", bs.BytesPerSector);
	Log_Debug("FS_NTFS", "SectorsPerCluster = %i", bs.SectorsPerCluster);
	Log_Debug("FS_NTFS", "MediaDescriptor = 0x%x", bs.MediaDescriptor);
	Log_Debug("FS_NTFS", "SectorsPerTrack = %i", bs.SectorsPerTrack);
	Log_Debug("FS_NTFS", "Heads = %i", bs.Heads);
	Log_Debug("FS_NTFS", "TotalSectorCount = 0x%llx", bs.TotalSectorCount);
	Log_Debug("FS_NTFS", "MFTStart = 0x%llx", bs.MFTStart);
	Log_Debug("FS_NTFS", "MFTMirrorStart = 0x%llx", bs.MFTMirrorStart);
	Log_Debug("FS_NTFS", "ClustersPerMFTRecord = %i", bs.ClustersPerMFTRecord);
	Log_Debug("FS_NTFS", "ClustersPerIndexRecord = %i", bs.ClustersPerIndexRecord);
	Log_Debug("FS_NTFS", "ClustersPerIndexRecord = %i", bs.ClustersPerIndexRecord);
	Log_Debug("FS_NTFS", "SerialNumber = 0x%llx", bs.SerialNumber);
	
	disk->ClusterSize = bs.BytesPerSector * bs.SectorsPerCluster;
	Log_Debug("NTFS", "Cluster Size = %i KiB", disk->ClusterSize/1024);
	disk->MFTBase = bs.MFTStart;
	Log_Debug("NTFS", "MFT Base = %i", disk->MFTBase);
	Log_Debug("NTFS", "TotalSectorCount = 0x%x", bs.TotalSectorCount);
	
	disk->RootNode.Inode = 5;	// MFT Ent #5 is filesystem root
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
	
	NTFS_DumpEntry(disk, 5);
	
	return &disk->RootNode;
}

/**
 * \brief Unmount an NTFS Disk
 */
void NTFS_Unmount(tVFS_Node *Node)
{
	
}

/**
 * \brief Dumps a MFT Entry
 */
void NTFS_DumpEntry(tNTFS_Disk *Disk, Uint32 Entry)
{
	void	*buf = malloc( Disk->ClusterSize );
	tNTFS_FILE_Header	*hdr = buf;
	
	if(!buf) {
		Log_Warning("FS_NTFS", "malloc() fail!");
		return ;
	}
	
	VFS_ReadAt( Disk->FD, Disk->MFTBase*Disk->ClusterSize, Disk->ClusterSize, buf);
	
	Log_Debug("FS_NTFS", "MFT Entry #%i", Entry);
	Log_Debug("FS_NTFS", "- Magic = 0x%08x (%4C)", hdr->Magic, &hdr->Magic);
	Log_Debug("FS_NTFS", "- UpdateSequenceOfs = 0x%x", hdr->UpdateSequenceOfs);
	Log_Debug("FS_NTFS", "- UpdateSequenceSize = 0x%x", hdr->UpdateSequenceSize);
	Log_Debug("FS_NTFS", "- LSN = 0x%x", hdr->LSN);
	Log_Debug("FS_NTFS", "- SequenceNumber = %i", hdr->SequenceNumber);
	Log_Debug("FS_NTFS", "- HardLinkCount = %i", hdr->HardLinkCount);
	Log_Debug("FS_NTFS", "- FirstAttribOfs = 0x%x", hdr->FirstAttribOfs);
	Log_Debug("FS_NTFS", "- Flags = 0x%x", hdr->Flags);
	Log_Debug("FS_NTFS", "- RecordSize = 0x%x", hdr->RecordSize);
	Log_Debug("FS_NTFS", "- RecordSpace = 0x%x", hdr->RecordSpace);
	Log_Debug("FS_NTFS", "- Reference = 0x%llx", hdr->Reference);
	Log_Debug("FS_NTFS", "- NextAttribID = 0x%04x", hdr->NextAttribID);
	
	free(buf);
}
