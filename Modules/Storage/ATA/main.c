/*
 * Acess2 IDE Harddisk Driver
 * - main.c
 */
#define DEBUG	0
#define VERSION	0x0032
#include <acess.h>
#include <modules.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <api_drv_common.h>
#include <api_drv_disk.h>
#include "common.h"

// === MACROS ===
#define IO_DELAY()	do{inb(0x80); inb(0x80); inb(0x80); inb(0x80);}while(0)

// === PROTOTYPES ===
 int	ATA_Install(char **Arguments);
void	ATA_SetupPartitions(void);
void	ATA_SetupVFS(void);
 int	ATA_ScanDisk(int Disk);
void	ATA_ParseGPT(int Disk);
void	ATA_int_MakePartition(tATA_Partition *Part, int Disk, int Num, Uint64 Start, Uint64 Length);
Uint16	ATA_GetBasePort(int Disk);
// Filesystem Interface
char	*ATA_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*ATA_FindDir(tVFS_Node *Node, const char *Name);
Uint64	ATA_ReadFS(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	ATA_WriteFS(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	ATA_IOCtl(tVFS_Node *Node, int Id, void *Data);
// Read/Write Interface/Quantiser
Uint	ATA_ReadRaw(Uint64 Address, Uint Count, void *Buffer, Uint Disk);
Uint	ATA_WriteRaw(Uint64 Address, Uint Count, void *Buffer, Uint Disk);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, i386ATA, ATA_Install, NULL, "PCI", NULL);
tDevFS_Driver	gATA_DriverInfo = {
	NULL, "ata",
	{
		.NumACLs = 1,
		.Size = -1,
		.Flags = VFS_FFLAG_DIRECTORY,
		.ACLs = &gVFS_ACL_EveryoneRX,
		.ReadDir = ATA_ReadDir,
		.FindDir = ATA_FindDir
	}
};
tATA_Disk	gATA_Disks[MAX_ATA_DISKS];
 int	giATA_NumNodes;
tVFS_Node	**gATA_Nodes;

// === CODE ===
/**
 * \brief Initialise the ATA driver
 */
int ATA_Install(char **Arguments)
{
	int	ret;

	ret = ATA_SetupIO();
	if(ret)	return ret;

	ATA_SetupPartitions();

	ATA_SetupVFS();

	if( DevFS_AddDevice( &gATA_DriverInfo ) == 0 )
		return MODULE_ERR_MISC;

	return MODULE_ERR_OK;
}

/**
 * \brief Scan all disks, looking for partitions
 */
void ATA_SetupPartitions(void)
{
	 int	i;
	for( i = 0; i < MAX_ATA_DISKS; i ++ )
	{
		if( !ATA_ScanDisk(i) ) {
			gATA_Disks[i].Name[0] = '\0';	// Mark as unused
			continue;
		}
	}
}

/**
 * \brief Sets up the ATA drivers VFS information and registers with DevFS
 */
void ATA_SetupVFS(void)
{
	 int	i, j, k;

	// Count number of nodes needed
	giATA_NumNodes = 0;
	for( i = 0; i < MAX_ATA_DISKS; i++ )
	{
		if(gATA_Disks[i].Name[0] == '\0')	continue;	// Ignore
		giATA_NumNodes ++;
		giATA_NumNodes += gATA_Disks[i].NumPartitions;
	}

	// Allocate Node space
	gATA_Nodes = malloc( giATA_NumNodes * sizeof(void*) );

	// Set nodes
	k = 0;
	for( i = 0; i < MAX_ATA_DISKS; i++ )
	{
		if(gATA_Disks[i].Name[0] == '\0')	continue;	// Ignore
		gATA_Nodes[ k++ ] = &gATA_Disks[i].Node;
		for( j = 0; j < gATA_Disks[i].NumPartitions; j ++ )
			gATA_Nodes[ k++ ] = &gATA_Disks[i].Partitions[j].Node;
	}

	gATA_DriverInfo.RootNode.Size = giATA_NumNodes;
}

/**
 * \brief Scan a disk, getting the size and any paritions
 * \param Disk	Disk ID to scan
 */
int ATA_ScanDisk(int Disk)
{
	tVFS_Node	*node;
	tMBR	mbr;

	ENTER("iDisk", Disk);
	
	// Get the disk size
	gATA_Disks[ Disk ].Sectors = ATA_GetDiskSize(Disk);
	if(gATA_Disks[ Disk ].Sectors == 0)
	{
		LEAVE('i', 0);
		return 0;
	}

	LOG("gATA_Disks[ %i ].Sectors = 0x%x", Disk, gATA_Disks[ Disk ].Sectors);

	// Create Name
	gATA_Disks[ Disk ].Name[0] = 'A'+Disk;
	gATA_Disks[ Disk ].Name[1] = '\0';

	#if 1
	{
		Uint64	val = gATA_Disks[ Disk ].Sectors / 2;
		char	*units = "KiB";
		if( val > 4*1024 ) {
			val /= 1024;
			units = "MiB";
		}
		else if( val > 4*1024 ) {
			val /= 1024;
			units = "GiB";
		}
		else if( val > 4*1024 ) {
			val /= 1024;
			units = "TiB";
		}
		Log_Notice("ATA", "Disk %s: 0x%llx Sectors (%lli %s)",
			gATA_Disks[ Disk ].Name, gATA_Disks[ Disk ].Sectors, val, units);
	}
	#endif

	// Get pointer to vfs node and populate it
	node = &gATA_Disks[ Disk ].Node;
	node->Size = gATA_Disks[Disk].Sectors * SECTOR_SIZE;
	node->NumACLs = 0;	// Means Superuser only can access it
	node->Inode = (Disk << 8) | 0xFF;
	node->ImplPtr = gATA_Disks[ Disk ].Name;

	node->ATime = node->MTime
		= node->CTime = now();

	node->Read = ATA_ReadFS;
	node->Write = ATA_WriteFS;
	node->IOCtl = ATA_IOCtl;

	// --- Scan Partitions ---
	LOG("Reading MBR");
	// Read Boot Sector
	if( ATA_ReadDMA( Disk, 0, 1, &mbr ) != 0 ) {
		Log_Warning("ATA", "Error in reading MBR on %i", Disk);
		LEAVE('i', 0);
		return 0;
	}

	// Check for a GPT table
	if(mbr.Parts[0].SystemID == 0xEE)
		ATA_ParseGPT(Disk);
	else	// No? Just parse the MBR
		ATA_ParseMBR(Disk, &mbr);
	
	#if DEBUG >= 2
	ATA_ReadDMA( Disk, 1, 1, &mbr );
	Debug_HexDump("ATA_ScanDisk", &mbr, 512);
	#endif

	LEAVE('i', 1);
	return 1;
}

/**
 * \fn void ATA_int_MakePartition(tATA_Partition *Part, int Disk, int Num, Uint64 Start, Uint64 Length)
 * \brief Fills a parition's information structure
 */
void ATA_int_MakePartition(tATA_Partition *Part, int Disk, int Num, Uint64 Start, Uint64 Length)
{
	ENTER("pPart iDisk iNum XStart XLength", Part, Disk, Num, Start, Length);
	Part->Start = Start;
	Part->Length = Length;
	Part->Name[0] = 'A'+Disk;
	if(Num >= 10) {
		Part->Name[1] = '1'+Num/10;
		Part->Name[2] = '1'+Num%10;
		Part->Name[3] = '\0';
	} else {
		Part->Name[1] = '1'+Num;
		Part->Name[2] = '\0';
	}
	Part->Node.NumACLs = 0;	// Only root can read/write raw block devices
	Part->Node.Inode = (Disk << 8) | Num;
	Part->Node.ImplPtr = Part->Name;

	Part->Node.Read = ATA_ReadFS;
	Part->Node.Write = ATA_WriteFS;
	Part->Node.IOCtl = ATA_IOCtl;
	Log_Notice("ATA", "Partition %s at 0x%llx+0x%llx", Part->Name, Part->Start, Part->Length);
	LOG("Made '%s' (&Node=%p)", Part->Name, &Part->Node);
	LEAVE('-');
}

/**
 * \fn void ATA_ParseGPT(int Disk)
 * \brief Parses the GUID Partition Table
 */
void ATA_ParseGPT(int Disk)
{
	///\todo Support GPT Disks
	Warning("GPT Disks are currently unsupported (Disk %i)", Disk);
}

/**
 * \fn char *ATA_ReadDir(tVFS_Node *Node, int Pos)
 */
char *ATA_ReadDir(tVFS_Node *UNUSED(Node), int Pos)
{
	if(Pos >= giATA_NumNodes || Pos < 0)	return NULL;
	return strdup( gATA_Nodes[Pos]->ImplPtr );
}

/**
 * \fn tVFS_Node *ATA_FindDir(tVFS_Node *Node, const char *Name)
 */
tVFS_Node *ATA_FindDir(tVFS_Node *UNUSED(Node), const char *Name)
{
	 int	part;
	tATA_Disk	*disk;
	
	// Check first character
	if(Name[0] < 'A' || Name[0] > 'A'+MAX_ATA_DISKS)
		return NULL;
	disk = &gATA_Disks[Name[0]-'A'];
	// Raw Disk
	if(Name[1] == '\0') {
		if( disk->Sectors == 0 && disk->Name[0] == '\0')
			return NULL;
		return &disk->Node;
	}

	// Partitions
	if(Name[1] < '0' || '9' < Name[1])	return NULL;
	if(Name[2] == '\0') {	// <= 9
		part = Name[1] - '0';
		part --;
		return &disk->Partitions[part].Node;
	}
	// > 9
	if('0' > Name[2] || '9' < Name[2])	return NULL;
	if(Name[3] != '\0')	return NULL;

	part = (Name[1] - '0') * 10;
	part += Name[2] - '0';
	part --;
	return &disk->Partitions[part].Node;

}

/**
 * \fn Uint64 ATA_ReadFS(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 */
Uint64 ATA_ReadFS(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	 int	disk = Node->Inode >> 8;
	 int	part = Node->Inode & 0xFF;

	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);

	// Raw Disk Access
	if(part == 0xFF)
	{
		if( Offset >= gATA_Disks[disk].Sectors * SECTOR_SIZE ) {
			LEAVE('i', 0);
			return 0;
		}
		if( Offset + Length > gATA_Disks[disk].Sectors*SECTOR_SIZE )
			Length = gATA_Disks[disk].Sectors*SECTOR_SIZE - Offset;
	}
	// Partition
	else
	{
		if( Offset >= gATA_Disks[disk].Partitions[part].Length * SECTOR_SIZE ) {
			LEAVE('i', 0);
			return 0;
		}
		if( Offset + Length > gATA_Disks[disk].Partitions[part].Length * SECTOR_SIZE )
			Length = gATA_Disks[disk].Partitions[part].Length * SECTOR_SIZE - Offset;
		Offset += gATA_Disks[disk].Partitions[part].Start * SECTOR_SIZE;
	}

	{
		int ret = DrvUtil_ReadBlock(Offset, Length, Buffer, ATA_ReadRaw, SECTOR_SIZE, disk);
		//Log("ATA_ReadFS: disk=%i, Offset=%lli, Length=%lli", disk, Offset, Length);
		//Debug_HexDump("ATA_ReadFS", Buffer, Length);
		LEAVE('i', ret);
		return ret;
	}
}

/**
 * \fn Uint64 ATA_WriteFS(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 */
Uint64 ATA_WriteFS(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	 int	disk = Node->Inode >> 8;
	 int	part = Node->Inode & 0xFF;

	// Raw Disk Access
	if(part == 0xFF)
	{
		if( Offset >= gATA_Disks[disk].Sectors * SECTOR_SIZE )
			return 0;
		if( Offset + Length > gATA_Disks[disk].Sectors*SECTOR_SIZE )
			Length = gATA_Disks[disk].Sectors*SECTOR_SIZE - Offset;
	}
	// Partition
	else
	{
		if( Offset >= gATA_Disks[disk].Partitions[part].Length * SECTOR_SIZE )
			return 0;
		if( Offset + Length > gATA_Disks[disk].Partitions[part].Length * SECTOR_SIZE )
			Length = gATA_Disks[disk].Partitions[part].Length * SECTOR_SIZE - Offset;
		Offset += gATA_Disks[disk].Partitions[part].Start * SECTOR_SIZE;
	}

	Log("ATA_WriteFS: (Node=%p, Offset=0x%llx, Length=0x%llx, Buffer=%p)", Node, Offset, Length, Buffer);
	Debug_HexDump("ATA_WriteFS", Buffer, Length);
	return DrvUtil_WriteBlock(Offset, Length, Buffer, ATA_ReadRaw, ATA_WriteRaw, SECTOR_SIZE, disk);
}

const char	*csaATA_IOCtls[] = {DRV_IOCTLNAMES, DRV_DISK_IOCTLNAMES, NULL};
/**
 * \fn int ATA_IOCtl(tVFS_Node *Node, int Id, void *Data)
 * \brief IO Control Funtion
 */
int ATA_IOCtl(tVFS_Node *UNUSED(Node), int Id, void *Data)
{
	switch(Id)
	{
	BASE_IOCTLS(DRV_TYPE_DISK, "i386ATA", VERSION, csaATA_IOCtls);
	
	case DISK_IOCTL_GETBLOCKSIZE:
		return 512;	
	
	default:
		return 0;
	}
	return 0;
}

// --- Disk Access ---
/**
 * \fn Uint ATA_ReadRaw(Uint64 Address, Uint Count, void *Buffer, Uint Disk)
 */
Uint ATA_ReadRaw(Uint64 Address, Uint Count, void *Buffer, Uint Disk)
{
	 int	ret;
	Uint	offset;
	Uint	done = 0;

	// Pass straight on to ATA_ReadDMAPage if we can
	if(Count <= MAX_DMA_SECTORS)
	{
		ret = ATA_ReadDMA(Disk, Address, Count, Buffer);
		if(ret == 0)	return 0;
		return Count;
	}

	// Else we will have to break up the transfer
	offset = 0;
	while(Count > MAX_DMA_SECTORS)
	{
		ret = ATA_ReadDMA(Disk, Address+offset, MAX_DMA_SECTORS, Buffer+offset);
		// Check for errors
		if(ret != 1)	return done;
		// Change Position
		done += MAX_DMA_SECTORS;
		Count -= MAX_DMA_SECTORS;
		offset += MAX_DMA_SECTORS*SECTOR_SIZE;
	}

	ret = ATA_ReadDMA(Disk, Address+offset, Count, Buffer+offset);
	if(ret != 1)	return 0;
	return done+Count;
}

/**
 * \fn Uint ATA_WriteRaw(Uint64 Address, Uint Count, void *Buffer, Uint Disk)
 */
Uint ATA_WriteRaw(Uint64 Address, Uint Count, void *Buffer, Uint Disk)
{
	 int	ret;
	Uint	offset;
	Uint	done = 0;

	// Pass straight on to ATA_WriteDMA, if we can
	if(Count <= MAX_DMA_SECTORS)
	{
		ret = ATA_WriteDMA(Disk, Address, Count, Buffer);
		if(ret == 0)	return 0;
		return Count;
	}

	// Else we will have to break up the transfer
	offset = 0;
	while(Count > MAX_DMA_SECTORS)
	{
		ret = ATA_WriteDMA(Disk, Address+offset, MAX_DMA_SECTORS, Buffer+offset);
		// Check for errors
		if(ret != 1)	return done;
		// Change Position
		done += MAX_DMA_SECTORS;
		Count -= MAX_DMA_SECTORS;
		offset += MAX_DMA_SECTORS*SECTOR_SIZE;
	}

	ret = ATA_WriteDMA(Disk, Address+offset, Count, Buffer+offset);
	if(ret != 1)	return 0;
	return done+Count;
}
