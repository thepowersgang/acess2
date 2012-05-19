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
#include <Storage/LVM/include/lvm.h>

// === MACROS ===
#define IO_DELAY()	do{inb(0x80); inb(0x80); inb(0x80); inb(0x80);}while(0)

// === PROTOTYPES ===
 int	ATA_Install(char **Arguments);
void	ATA_SetupPartitions(void);
 int	ATA_ScanDisk(int Disk);
void	ATA_ParseGPT(int Disk);
void	ATA_int_MakePartition(tATA_Partition *Part, int Disk, int Num, Uint64 Start, Uint64 Length);
Uint16	ATA_GetBasePort(int Disk);
// Read/Write Interface/Quantiser
int	ATA_ReadRaw(void *Ptr, Uint64 Address, size_t Count, void *Buffer);
int	ATA_WriteRaw(void *Ptr, Uint64 Address, size_t Count, const void *Buffer);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, i386ATA, ATA_Install, NULL, "PCI", "LVM", NULL);
tLVM_VolType	gATA_VolType = {
	.Name = "ATA",
	.Read = ATA_ReadRaw,
	.Write = ATA_WriteRaw
	};

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
			continue;
		}
	}
}

/**
 * \brief Scan a disk, getting the size and any paritions
 * \param Disk	Disk ID to scan
 */
int ATA_ScanDisk(int Disk)
{
	Uint64	sector_count;
	ENTER("iDisk", Disk);
	
	// Get the disk size
	sector_count = ATA_GetDiskSize(Disk);
	if(sector_count == 0)
	{
		LEAVE('i', 0);
		return 0;
	}

	#if 1
	{
		Uint64	val = sector_count / 2;
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
		Log_Notice("ATA", "Disk %i: 0x%llx Sectors (%lli %s)",
			Disk, sector_count, val, units);
	}
	#endif

	char name[] = "ata0";
	sprintf(name, "ata%i", Disk);
	LVM_AddVolume(&gATA_VolType, name, (void*)(Uint*)Disk, 512, sector_count);

	#if DEBUG >= 2
	{
	char	mbr[512];
	ATA_ReadDMA( Disk, 1, 1, &mbr );
	Debug_HexDump("ATA_ScanDisk", &mbr, 512);
	}
	#endif

	LEAVE('i', 1);
	return 1;
}

// --- Disk Access ---
/**
 * \fn Uint ATA_ReadRaw(Uint64 Address, Uint Count, void *Buffer, Uint Disk)
 */
int ATA_ReadRaw(void *Ptr, Uint64 Address, Uint Count, void *Buffer)
{
	 int	Disk = (tVAddr)Ptr;
	 int	ret;
	Uint	offset;
	Uint	done = 0;

	LOG("Reading %i sectors from 0x%llx of disk %i", Count, Address, Disk);

	// Pass straight on to ATA_ReadDMAPage if we can
	if(Count <= MAX_DMA_SECTORS)
	{
		ret = ATA_ReadDMA(Disk, Address, Count, Buffer);
		if(ret)	return 0;
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

	ret = ATA_ReadDMA(Disk, Address+done, Count, Buffer+offset);
	if(ret)	return 0;
	return done+Count;
}

/**
 * \fn Uint ATA_WriteRaw(Uint64 Address, Uint Count, const void *Buffer, Uint Disk)
 */
int ATA_WriteRaw(void *Ptr, Uint64 Address, Uint Count, const void *Buffer)
{
	 int	Disk = (tVAddr)Ptr;
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
