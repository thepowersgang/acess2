/*
 * Acess2 IDE Harddisk Driver
 * - main.c
 */
#define DEBUG	1
#include <acess.h>
#include <modules.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <tpl_drv_common.h>
#include <tpl_drv_disk.h>
#include "common.h"

// --- Flags ---
#define START_BEFORE_CMD	0

// === STRUCTURES ===
typedef struct
{
	Uint32	PBufAddr;
	Uint16	Bytes;
	Uint16	Flags;
} __attribute__ ((packed))	tPRDT_Ent;
typedef struct
{
	Uint16	Flags;		// 1
	Uint16	Usused1[9];	// 10
	char	SerialNum[20];	// 20
	Uint16	Usused2[3];	// 23
	char	FirmwareVer[8];	// 27
	char	ModelNumber[40];	// 47
	Uint16	SectPerInt;	// 48 - AND with 0xFF to get true value;
	Uint16	Unused3;	// 49
	Uint16	Capabilities[2];	// 51
	Uint16	Unused4[2];	// 53
	Uint16	ValidExtData;	// 54
	Uint16	Unused5[5];	 // 59
	Uint16	SizeOfRWMultiple;	// 60
	Uint32	Sectors28;	// 62
	Uint16	Unused6[100-62];
	Uint64	Sectors48;
	Uint16	Unused7[256-104];
} __attribute__ ((packed))	tIdentify;

// === IMPORTS ===
extern void	ATA_ParseMBR(int Disk);

// === PROTOTYPES ===
 int	ATA_Install();
 int	ATA_SetupIO();
void	ATA_SetupPartitions();
void	ATA_SetupVFS();
 int	ATA_ScanDisk(int Disk);
void	ATA_ParseGPT(int Disk);
void	ATA_int_MakePartition(tATA_Partition *Part, int Disk, int Num, Uint64 Start, Uint64 Length);
Uint16	ATA_GetBasePort(int Disk);
// Filesystem Interface
char	*ATA_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*ATA_FindDir(tVFS_Node *Node, char *Name);
Uint64	ATA_ReadFS(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	ATA_WriteFS(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	ATA_IOCtl(tVFS_Node *Node, int Id, void *Data);
// Read/Write Interface/Quantiser
Uint	ATA_ReadRaw(Uint64 Address, Uint Count, void *Buffer, Uint Disk);
Uint	ATA_WriteRaw(Uint64 Address, Uint Count, void *Buffer, Uint Disk);
// Read/Write DMA
 int	ATA_ReadDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer);
 int	ATA_WriteDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer);
// IRQs
void	ATA_IRQHandlerPri(int unused);
void	ATA_IRQHandlerSec(int unused);
// Controller IO
Uint8	ATA_int_BusMasterReadByte(int Ofs);
void	ATA_int_BusMasterWriteByte(int Ofs, Uint8 Value);
void	ATA_int_BusMasterWriteDWord(int Ofs, Uint32 Value);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0032, i386ATA, ATA_Install, NULL, NULL);
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
Uint16	gATA_BusMasterBase = 0;
Uint8	*gATA_BusMasterBasePtr = 0;
 int	gATA_IRQPri = 14;
 int	gATA_IRQSec = 15;
 int	giaATA_ControllerLock[2] = {0};	//!< Spinlocks for each controller
Uint8	gATA_Buffers[2][4096] __attribute__ ((section(".padata")));
 int	gaATA_IRQs[2] = {0};
tPRDT_Ent	gATA_PRDTs[2] = {
	{0, 512, IDE_PRDT_LAST},
	{0, 512, IDE_PRDT_LAST}
};

// === CODE ===
/**
 * \fn int ATA_Install()
 */
int ATA_Install()
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
 * \fn int ATA_SetupIO()
 * \brief Sets up the ATA controller's DMA mode
 */
int ATA_SetupIO()
{
	 int	ent;
	tPAddr	addr;

	ENTER("");

	// Get IDE Controller's PCI Entry
	ent = PCI_GetDeviceByClass(0x0101, 0xFFFF, -1);
	LOG("ent = %i", ent);
	gATA_BusMasterBase = PCI_GetBAR4( ent );
	if( gATA_BusMasterBase == 0 ) {
		Warning("It seems that there is no Bus Master Controller on this machine. Get one");
		LEAVE('i', MODULE_ERR_NOTNEEDED);
		return MODULE_ERR_NOTNEEDED;
	}
	
	// Map memory
	if( !(gATA_BusMasterBase & 1) )
	{
		if( gATA_BusMasterBase < 0x100000 )
			gATA_BusMasterBasePtr = (void*)(0xC0000000|gATA_BusMasterBase);
		else
			gATA_BusMasterBasePtr = (void*)( MM_MapHWPage( gATA_BusMasterBase, 1 ) + (gATA_BusMasterBase&0xFFF) );
		LOG("gATA_BusMasterBasePtr = %p", gATA_BusMasterBasePtr);
	}
	else {
		// Bit 0 is left set as a flag to other functions
		LOG("gATA_BusMasterBase = 0x%x", gATA_BusMasterBase & ~1);
	}

	// Register IRQs and get Buffers
	IRQ_AddHandler( gATA_IRQPri, ATA_IRQHandlerPri );
	IRQ_AddHandler( gATA_IRQSec, ATA_IRQHandlerSec );

	gATA_PRDTs[0].PBufAddr = MM_GetPhysAddr( (Uint)&gATA_Buffers[0] );
	gATA_PRDTs[1].PBufAddr = MM_GetPhysAddr( (Uint)&gATA_Buffers[1] );

	LOG("gATA_PRDTs = {PBufAddr: 0x%x, PBufAddr: 0x%x}", gATA_PRDTs[0].PBufAddr, gATA_PRDTs[1].PBufAddr);

	addr = MM_GetPhysAddr( (Uint)&gATA_PRDTs[0] );
	LOG("addr = 0x%x", addr);
	ATA_int_BusMasterWriteDWord(4, addr);
	addr = MM_GetPhysAddr( (Uint)&gATA_PRDTs[1] );
	LOG("addr = 0x%x", addr);
	ATA_int_BusMasterWriteDWord(12, addr);

	// Enable controllers
	outb(IDE_PRI_BASE+1, 1);
	outb(IDE_SEC_BASE+1, 1);

	// return
	LEAVE('i', MODULE_ERR_OK);
	return MODULE_ERR_OK;
}

/**
 * \fn void ATA_SetupPartitions()
 */
void ATA_SetupPartitions()
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
 * \fn void ATA_SetupVFS()
 * \brief Sets up the ATA drivers VFS information and registers with DevFS
 */
void ATA_SetupVFS()
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
 * \fn int ATA_ScanDisk(int Disk)
 */
int ATA_ScanDisk(int Disk)
{
	Uint16	buf[256];
	tIdentify	*identify = (void*)buf;
	tMBR	*mbr = (void*)buf;
	Uint16	base;
	Uint8	val;
	 int	i;
	tVFS_Node	*node;

	ENTER("iDisk", Disk);

	base = ATA_GetBasePort( Disk );

	LOG("base = 0x%x", base);

	// Send Disk Selector
	if(Disk == 1 || Disk == 3)
		outb(base+6, 0xB0);
	else
		outb(base+6, 0xA0);

	// Send IDENTIFY
	outb(base+7, 0xEC);
	val = inb(base+7);	// Read status
	if(val == 0) {
		LEAVE('i', 0);
		return 0;	// Disk does not exist
	}

	// Poll until BSY clears and DRQ sets or ERR is set
	while( ((val & 0x80) || !(val & 0x08)) && !(val & 1))	val = inb(base+7);

	if(val & 1) {
		LEAVE('i', 0);
		return 0;	// Error occured, so return false
	}

	// Read Data
	for(i=0;i<256;i++)	buf[i] = inw(base);

	// Populate Disk Structure
	if(identify->Sectors48 != 0)
		gATA_Disks[ Disk ].Sectors = identify->Sectors48;
	else
		gATA_Disks[ Disk ].Sectors = identify->Sectors28;


	LOG("gATA_Disks[ Disk ].Sectors = 0x%x", gATA_Disks[ Disk ].Sectors);

	if( gATA_Disks[ Disk ].Sectors / (2048*1024) )
		Log("Disk %i: 0x%llx Sectors (%i GiB)", Disk,
			gATA_Disks[ Disk ].Sectors, gATA_Disks[ Disk ].Sectors / (2048*1024));
	else if( gATA_Disks[ Disk ].Sectors / 2048 )
		Log("Disk %i: 0x%llx Sectors (%i MiB)", Disk,
			gATA_Disks[ Disk ].Sectors, gATA_Disks[ Disk ].Sectors / 2048);
	else
		Log("Disk %i: 0x%llx Sectors (%i KiB)", Disk,
			gATA_Disks[ Disk ].Sectors, gATA_Disks[ Disk ].Sectors / 2);

	// Create Name
	gATA_Disks[ Disk ].Name[0] = 'A'+Disk;
	gATA_Disks[ Disk ].Name[1] = '\0';

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
	ATA_ReadDMA( Disk, 0, 1, mbr );

	// Check for a GPT table
	if(mbr->Parts[0].SystemID == 0xEE)
		ATA_ParseGPT(Disk);
	else	// No? Just parse the MBR
		ATA_ParseMBR(Disk);

	LEAVE('i', 0);
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
	Warning("GPT Disks are currently unsupported");
}

/**
 * \fn Uint16 ATA_GetPortBase(int Disk)
 * \brief Returns the base port for a given disk
 */
Uint16 ATA_GetBasePort(int Disk)
{
	switch(Disk)
	{
	case 0:	case 1:		return IDE_PRI_BASE;
	case 2:	case 3:		return IDE_SEC_BASE;
	}
	return 0;
}

/**
 * \fn char *ATA_ReadDir(tVFS_Node *Node, int Pos)
 */
char *ATA_ReadDir(tVFS_Node *Node, int Pos)
{
	if(Pos >= giATA_NumNodes || Pos < 0)	return NULL;
	return strdup( gATA_Nodes[Pos]->ImplPtr );
}

/**
 * \fn tVFS_Node *ATA_FindDir(tVFS_Node *Node, char *Name)
 */
tVFS_Node *ATA_FindDir(tVFS_Node *Node, char *Name)
{
	 int	part;
	// Check first character
	if(Name[0] < 'A' || Name[0] > 'A'+MAX_ATA_DISKS)
		return NULL;
	// Raw Disk
	if(Name[1] == '\0') {
		if( gATA_Disks[Name[0]-'A'].Sectors == 0 )
			return NULL;
		return &gATA_Disks[Name[0]-'A'].Node;
	}

	// Partitions
	if(Name[1] < '0' || '9' < Name[1])	return NULL;
	if(Name[2] == '\0') {	// <= 9
		part = Name[1] - '0';
		part --;
		return &gATA_Disks[Name[0]-'A'].Partitions[part].Node;
	}
	// > 9
	if('0' > Name[2] || '9' < Name[2])	return NULL;
	if(Name[3] != '\0')	return NULL;

	part = (Name[1] - '0') * 10;
	part += Name[2] - '0';
	part --;
	return &gATA_Disks[Name[0]-'A'].Partitions[part].Node;

}

/**
 * \fn Uint64 ATA_ReadFS(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 */
Uint64 ATA_ReadFS(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
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

	//Log("ATA_ReadFS: (Node=%p, Offset=0x%llx, Length=0x%llx, Buffer=%p)", Node, Offset, Length, Buffer);
	return DrvUtil_ReadBlock(Offset, Length, Buffer, ATA_ReadRaw, SECTOR_SIZE, disk);
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

/**
 * \fn int ATA_IOCtl(tVFS_Node *Node, int Id, void *Data)
 * \brief IO Control Funtion
 */
int ATA_IOCtl(tVFS_Node *Node, int Id, void *Data)
{
	switch(Id)
	{
	case DRV_IOCTL_TYPE:	return DRV_TYPE_DISK;
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

	// Pass straight on to ATA_WriteDMA if we can
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

/**
 * \fn int ATA_ReadDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer)
 */
int ATA_ReadDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer)
{
	 int	cont = (Disk>>1)&1;	// Controller ID
	 int	disk = Disk & 1;
	Uint16	base;

	ENTER("iDisk XAddress iCount pBuffer", Disk, Address, Count, Buffer);

	// Check if the count is small enough
	if(Count > MAX_DMA_SECTORS) {
		Warning("Passed too many sectors for a bulk DMA read (%i > %i)",
			Count, MAX_DMA_SECTORS);
		LEAVE('i');
		return 0;
	}

	// Get exclusive access to the disk controller
	LOCK( &giaATA_ControllerLock[ cont ] );

	// Set Size
	gATA_PRDTs[ cont ].Bytes = Count * SECTOR_SIZE;

	// Get Port Base
	base = ATA_GetBasePort(Disk);

	// Reset IRQ Flag
	gaATA_IRQs[cont] = 0;

	// Set up transfer
	outb(base+0x01, 0x00);
	if( Address > 0x0FFFFFFF )	// Use LBA48
	{
		outb(base+0x6, 0x40 | (disk << 4));
		outb(base+0x2, 0 >> 8);	// Upper Sector Count
		outb(base+0x3, Address >> 24);	// Low 2 Addr
		outb(base+0x3, Address >> 28);	// Mid 2 Addr
		outb(base+0x3, Address >> 32);	// High 2 Addr
	}
	else
	{
		outb(base+0x06, 0xE0 | (disk << 4) | ((Address >> 24) & 0x0F));	//Disk,Magic,High addr
	}

	outb(base+0x02, (Uint8) Count);		// Sector Count
	outb(base+0x03, (Uint8) Address);		// Low Addr
	outb(base+0x04, (Uint8) (Address >> 8));	// Middle Addr
	outb(base+0x05, (Uint8) (Address >> 16));	// High Addr

	LOG("Starting Transfer");
	#if START_BEFORE_CMD
	// Start transfer
	ATA_int_BusMasterWriteByte( cont << 3, 9 );	// Read and start
	if( Address > 0x0FFFFFFF )
		outb(base+0x07, HDD_DMA_R48);	// Read Command (LBA48)
	else
		outb(base+0x07, HDD_DMA_R28);	// Read Command (LBA28)
	#else
	if( Address > 0x0FFFFFFF )
		outb(base+0x07, HDD_DMA_R48);	// Read Command (LBA48)
	else
		outb(base+0x07, HDD_DMA_R28);	// Read Command (LBA28)
	// Start transfer
	ATA_int_BusMasterWriteByte( cont << 3, 9 );	// Read and start
	#endif

	// Wait for transfer to complete
	//ATA_int_BusMasterWriteByte( (cont << 3) + 2, 0x4 );
	while( gaATA_IRQs[cont] == 0 ) {
		//Uint8	val = ATA_int_BusMasterReadByte( (cont << 3) + 2, 0x4 );
		//LOG("val = 0x%02x", val);
		Threads_Yield();
	}

	// Complete Transfer
	ATA_int_BusMasterWriteByte( cont << 3, 0 );	// Write and stop

	LOG("Transfer Completed & Acknowledged");

	// Copy to destination buffer
	memcpy( Buffer, gATA_Buffers[cont], Count*SECTOR_SIZE );

	// Release controller lock
	RELEASE( &giaATA_ControllerLock[ cont ] );

	LEAVE('i', 1);
	return 1;
}

/**
 * \fn int ATA_WriteDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer)
 */
int ATA_WriteDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer)
{
	 int	cont = (Disk>>1)&1;	// Controller ID
	 int	disk = Disk & 1;
	Uint16	base;

	// Check if the count is small enough
	if(Count > MAX_DMA_SECTORS)	return 0;

	// Get exclusive access to the disk controller
	LOCK( &giaATA_ControllerLock[ cont ] );

	// Set Size
	gATA_PRDTs[ cont ].Bytes = Count * SECTOR_SIZE;

	// Get Port Base
	base = ATA_GetBasePort(Disk);

	// Set up transfer
	outb(base+0x01, 0x00);
	if( Address > 0x0FFFFFFF )	// Use LBA48
	{
		outb(base+0x6, 0x40 | (disk << 4));
		outb(base+0x2, 0 >> 8);	// Upper Sector Count
		outb(base+0x3, Address >> 24);	// Low 2 Addr
		outb(base+0x3, Address >> 28);	// Mid 2 Addr
		outb(base+0x3, Address >> 32);	// High 2 Addr
	}
	else
	{
		outb(base+0x06, 0xE0 | (disk << 4) | ((Address >> 24) & 0x0F));	//Disk,Magic,High addr
	}

	outb(base+0x02, (Uint8) Count);		// Sector Count
	outb(base+0x03, (Uint8) Address);		// Low Addr
	outb(base+0x04, (Uint8) (Address >> 8));	// Middle Addr
	outb(base+0x05, (Uint8) (Address >> 16));	// High Addr
	if( Address > 0x0FFFFFFF )
		outb(base+0x07, HDD_DMA_W48);	// Write Command (LBA48)
	else
		outb(base+0x07, HDD_DMA_W28);	// Write Command (LBA28)

	// Reset IRQ Flag
	gaATA_IRQs[cont] = 0;

	// Copy to output buffer
	memcpy( gATA_Buffers[cont], Buffer, Count*SECTOR_SIZE );

	// Start transfer
	ATA_int_BusMasterWriteByte( cont << 3, 1 );	// Write and start

	// Wait for transfer to complete
	while( gaATA_IRQs[cont] == 0 )	Threads_Yield();

	// Complete Transfer
	ATA_int_BusMasterWriteByte( cont << 3, 0 );	// Write and stop

	// Release controller lock
	RELEASE( &giaATA_ControllerLock[ cont ] );

	return 1;
}

/**
 * \fn void ATA_IRQHandlerPri(int unused)
 */
void ATA_IRQHandlerPri(int unused)
{
	Uint8	val;

	// IRQ bit set for Primary Controller
	val = ATA_int_BusMasterReadByte( 0x2 );
	LOG("IRQ val = 0x%x", val);
	if(val & 4) {
		LOG("IRQ hit (val = 0x%x)", val);
		ATA_int_BusMasterWriteByte( 0x2, 4 );
		gaATA_IRQs[0] = 1;
		return ;
	}
}

/**
 * \fn void ATA_IRQHandlerSec(int unused)
 */
void ATA_IRQHandlerSec(int unused)
{
	Uint8	val;
	// IRQ bit set for Secondary Controller
	val = ATA_int_BusMasterReadByte( 0xA );
	LOG("IRQ val = 0x%x", val);
	if(val & 4) {
		LOG("IRQ hit (val = 0x%x)", val);
		ATA_int_BusMasterWriteByte( 0xA, 4 );
		gaATA_IRQs[1] = 1;
		return ;
	}
}

/**
 * \fn Uint8 ATA_int_BusMasterReadByte(int Ofs)
 */
Uint8 ATA_int_BusMasterReadByte(int Ofs)
{
	if( gATA_BusMasterBase & 1 )
		return inb( (gATA_BusMasterBase & ~1) + Ofs );
	else
		return *(Uint8*)(gATA_BusMasterBasePtr + Ofs);
}

/**
 * \fn void ATA_int_BusMasterWriteByte(int Ofs, Uint8 Value)
 * \brief Writes a byte to a Bus Master Register
 */
void ATA_int_BusMasterWriteByte(int Ofs, Uint8 Value)
{
	if( gATA_BusMasterBase & 1 )
		outb( (gATA_BusMasterBase & ~1) + Ofs, Value );
	else
		*(Uint8*)(gATA_BusMasterBasePtr + Ofs) = Value;
}

/**
 * \fn void ATA_int_BusMasterWriteDWord(int Ofs, Uint32 Value)
 * \brief Writes a dword to a Bus Master Register
 */
void ATA_int_BusMasterWriteDWord(int Ofs, Uint32 Value)
{

	if( gATA_BusMasterBase & 1 )
		outd( (gATA_BusMasterBase & ~1) + Ofs, Value );
	else
		*(Uint32*)(gATA_BusMasterBasePtr + Ofs) = Value;
}
