/*
 * Acess2 82077AA FDC
 * - By John Hodge (thePowersGang)
 *
 * fdc.c
 * - Core file
 */
#define DEBUG	0
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include "common.h"
#include <api_drv_disk.h>

// === CONSTANTS ===
#define FDD_VERSION	VER2(1,10)

// === STRUCTURES ===

// === PROTOTYPES ===
 int	FDD_Install(char **Arguments);
 int	FDD_RegisterFS(void);
// --- VFS
char	*FDD_ReadDir(tVFS_Node *Node, int pos);
tVFS_Node	*FDD_FindDir(tVFS_Node *dirNode, const char *Name);
 int	FDD_IOCtl(tVFS_Node *Node, int ID, void *Data);
Uint64	FDD_ReadFS(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer);
// --- Helpers
 int	FDD_int_ReadWriteWithinTrack(int Disk, int Track, int bWrite, size_t Offset, size_t Length, void *Buffer);

// === GLOBALS ===
MODULE_DEFINE(0, FDD_VERSION, Storage_FDDv2, FDD_Install, NULL, "x86_ISADMA", NULL);
tDrive	gaFDD_Disks[MAX_DISKS];
tVFS_Node	gaFDD_DiskNodes[MAX_DISKS];
tDevFS_Driver	gFDD_DriverInfo = {
	NULL, "fdd",
	{
	.Size = -1,
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.ReadDir = FDD_ReadDir,
	.FindDir = FDD_FindDir,
	.IOCtl = FDD_IOCtl
	}
	};

// === CODE ===
int FDD_Install(char **Arguments)
{
	// Query CMOS memory
	{
		Uint8	data;
		outb(0x70, 0x10);
		data = inb(0x71);
		
		// NOTE: CMOS only reports 2 disks
		if( (data & 0xF0) == 0x40 )
			gaFDD_Disks[0].bValid = gaFDD_Disks[0].bInserted = 1;
		if( (data & 0x0F) == 0x04 )
			gaFDD_Disks[1].bValid = gaFDD_Disks[1].bInserted = 1;
		
		if( gaFDD_Disks[0].bValid == 0 && gaFDD_Disks[1].bValid == 0 )
			return MODULE_ERR_NOTNEEDED;
	}
	
	// Initialise controller
	FDD_SetupIO();

	FDD_RegisterFS();

	return 0;
}

/**
 * \brief Register the FDD driver with DevFS
 */
int FDD_RegisterFS(void)
{
	gFDD_DriverInfo.RootNode.CTime = gFDD_DriverInfo.RootNode.MTime
		= gFDD_DriverInfo.RootNode.ATime = now();

	for( int i = 0; i < MAX_DISKS; i ++ )
	{
		if( !gaFDD_Disks[i].bValid )	continue ;
	
		// Initialise Child Nodes
		gaFDD_DiskNodes[i].Inode = i;
		gaFDD_DiskNodes[i].Flags = 0;
		gaFDD_DiskNodes[i].NumACLs = 0;
		gaFDD_DiskNodes[i].Read = FDD_ReadFS;
		gaFDD_DiskNodes[i].Write = NULL;//FDD_WriteFS;
		gaFDD_DiskNodes[i].Size = 1440*1024;	// TODO: Non 1.44 disks
	}
	
	DevFS_AddDevice( &gFDD_DriverInfo );
	return 0;
}

/**
 * \brief Get the name of the \a Pos th item in the driver root
 * \param Node	Root node (unused)
 * \param Pos	Position
 * \return Heap string of node name
 */
char *FDD_ReadDir(tVFS_Node *Node, int Pos)
{
	char	ret_tpl[2];
	if(Pos < 0 || Pos > MAX_DISKS )
		return NULL;
	if(gaFDD_Disks[Pos].bValid)
		return VFS_SKIP;
	
	ret_tpl[0] = '0' + Pos;
	ret_tpl[1] = '\0';
	return strdup(ret_tpl);
}

/**
 * \brief Get a node by name
 * \param Node	Root node (unused)
 * \param Name	Drive name
 * \return Pointer to node structure
 */
tVFS_Node *FDD_FindDir(tVFS_Node *Node, const char *Name)
{
	 int	pos;
	if( '0' > Name[0] || Name[0] > '9' )	return NULL;
	if( Name[1] != '\0' )	return NULL;
	
	pos = Name[0] - '0';
	
	return &gaFDD_DiskNodes[pos];
}

static const char	*casIOCTLS[] = {DRV_IOCTLNAMES,DRV_DISK_IOCTLNAMES,NULL};
/**
 * \brief Driver root IOCtl Handler
 * \param Node	Root node (unused)
 * \param ID	IOCtl ID
 * \param Data	IOCtl specific data pointer
 */
int FDD_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_DISK, "FDDv2", FDD_VERSION, casIOCTLS);
	
	case DISK_IOCTL_GETBLOCKSIZE:	return 512;
	
	default:
		return -1;
	}
}

/**
 * \brief Read from a disk
 * \param Node	Disk node
 * \param Offset	Byte offset in disk
 * \param Length	Number of bytes to read
 * \param Buffer	Destination buffer
 * \return Number of bytes read
 */
Uint64 FDD_ReadFS(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	 int	disk = Node->Inode;
	 int	track;
	 int	rem_len;
	char	*dest = Buffer;

	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);

	if( Offset > Node->Size )	LEAVE_RET('i', 0);
	if( Length > Node->Size )	Length = Node->Size;
	if( Offset + Length > Node->Size )
		Length = Node->Size - Offset;
	if( Length == 0 )	return 0;

	rem_len = Length;

	track = Offset / BYTES_PER_TRACK;
	Offset %= BYTES_PER_TRACK;	

	if( Offset )
	{
		 int	len;
		if(rem_len > BYTES_PER_TRACK-Offset)
			len = BYTES_PER_TRACK - Offset;
		else
			len = rem_len;
		FDD_int_ReadWriteWithinTrack(disk, track, 0, Offset, len, dest);
		dest += len;
		rem_len -= len;
		track ++;
	}

	while( rem_len > BYTES_PER_TRACK )
	{
		FDD_int_ReadWriteWithinTrack(disk, track, 0, 0, BYTES_PER_TRACK, dest);
		dest += BYTES_PER_TRACK;
		rem_len -= BYTES_PER_TRACK;
		track ++;
	}

	FDD_int_ReadWriteWithinTrack(disk, track, 0, 0, rem_len, dest);
	
	LEAVE('X', Length);
	return Length;
}

/**
 * \brief Read from a track
 */
int FDD_int_ReadWriteWithinTrack(int Disk, int Track, int bWrite, size_t Offset, size_t Length, void *Buffer)
{
	if( Offset > BYTES_PER_TRACK || Length > BYTES_PER_TRACK )
		return 1;
	if( Offset + Length > BYTES_PER_TRACK )
		return 1;

	ENTER("iDisk iTrack bbWrite xOffset xLength pBuffer",
		Disk, Track, bWrite, Offset, Length, Buffer);
	
	Mutex_Acquire( &gaFDD_Disks[Disk].Mutex );

	// If the cache doesn't exist, create it
	if( !gaFDD_Disks[Disk].TrackData[Track] )
	{
		gaFDD_Disks[Disk].TrackData[Track] = malloc( BYTES_PER_TRACK );
		// Don't bother reading if this is a whole track write
		if( !(bWrite && Offset == 0 && Length == BYTES_PER_TRACK) )
		{
			LOG("Reading track");
			FDD_int_ReadWriteTrack(Disk, Track, 0, gaFDD_Disks[Disk].TrackData[Track]);
		}
	}
	
	// Read/Write
	if( bWrite )
	{
		// Write to cache then commit cache to disk
		char	*dest = gaFDD_Disks[Disk].TrackData[Track];
		LOG("Write to cache");
		memcpy( dest + Offset, Buffer, Length );
		FDD_int_ReadWriteTrack(Disk, Track, 1, gaFDD_Disks[Disk].TrackData[Track]);
	}
	else
	{
		// Read from cache
		char	*src = gaFDD_Disks[Disk].TrackData[Track];
		LOG("Read from cache");
		memcpy(Buffer, src + Offset, Length);
	}
	
	Mutex_Release( &gaFDD_Disks[Disk].Mutex );

	LEAVE('i', 0);
	return 0;
}
