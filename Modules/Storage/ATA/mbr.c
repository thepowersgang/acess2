/*
 * Acess2 IDE Harddisk Driver
 * - MBR Parsing Code
 * mbr.c
 */
#define DEBUG	0
#include <acess.h>
#include "common.h"

// === PROTOTYPES ===
void	ATA_ParseMBR(int Disk, tMBR *MBR);
Uint64	ATA_MBR_int_ReadExt(int Disk, Uint64 Addr, Uint64 *Base, Uint64 *Length);

// === GLOBALS ===

// === CODE ===
/**
 * \fn void ATA_ParseMBR(int Disk, tMBR *MBR)
 */
void ATA_ParseMBR(int Disk, tMBR *MBR)
{
	 int	i, j = 0, k = 4;
	Uint64	extendedLBA;
	Uint64	base, len;
	
	ENTER("iDisk", Disk);
	
	// Count Partitions
	gATA_Disks[Disk].NumPartitions = 0;
	extendedLBA = 0;
	for( i = 0; i < 4; i ++ )
	{
		if( MBR->Parts[i].SystemID == 0 )	continue;
		if(	MBR->Parts[i].Boot == 0x0 || MBR->Parts[i].Boot == 0x80	// LBA 28
		||	MBR->Parts[i].Boot == 0x1 || MBR->Parts[i].Boot == 0x81	// LBA 48
			)
		{
			if( MBR->Parts[i].SystemID == 0xF || MBR->Parts[i].SystemID == 5 ) {
				LOG("Extended Partition at 0x%llx", MBR->Parts[i].LBAStart);
				if(extendedLBA != 0) {
					Warning("Disk %i has multiple extended partitions, ignoring rest", Disk);
					continue;
				}
				extendedLBA = MBR->Parts[i].LBAStart;
				continue;
			}
			LOG("Primary Partition at 0x%llx", MBR->Parts[i].LBAStart);
			
			gATA_Disks[Disk].NumPartitions ++;
			continue;
		}
		// Invalid Partition, so don't count it
	}
	while(extendedLBA != 0)
	{
		extendedLBA = ATA_MBR_int_ReadExt(Disk, extendedLBA, &base, &len);
		if( extendedLBA == -1 )	break;
		gATA_Disks[Disk].NumPartitions ++;
	}
	LOG("gATA_Disks[Disk].NumPartitions = %i", gATA_Disks[Disk].NumPartitions);
	
	// Create patition array
	gATA_Disks[Disk].Partitions = malloc( gATA_Disks[Disk].NumPartitions * sizeof(tATA_Partition) );
	
	// --- Fill Partition Info ---
	extendedLBA = 0;
	for( j = 0, i = 0; i < 4; i ++ )
	{
		LOG("MBR->Parts[%i].SystemID = 0x%02x", i, MBR->Parts[i].SystemID);
		if( MBR->Parts[i].SystemID == 0 )	continue;
		if( MBR->Parts[i].Boot == 0x0 || MBR->Parts[i].Boot == 0x80 )	// LBA 28
		{
			base = MBR->Parts[i].LBAStart;
			len = MBR->Parts[i].LBALength;
		}
		else if( MBR->Parts[i].Boot == 0x1 || MBR->Parts[i].Boot == 0x81 )	// LBA 58
		{
			base = (MBR->Parts[i].StartHi << 16) | MBR->Parts[i].LBAStart;
			len = (MBR->Parts[i].LengthHi << 16) | MBR->Parts[i].LBALength;
		}
		else
			continue;
		
		if( MBR->Parts[i].SystemID == 0xF || MBR->Parts[i].SystemID == 5 ) {
			if(extendedLBA != 0) {
				Log_Warning("ATA", "Disk %i has multiple extended partitions, ignoring rest", Disk);
				continue;
			}
			extendedLBA = base;
			continue;
		}
		// Create Partition
		ATA_int_MakePartition(
			&gATA_Disks[Disk].Partitions[j], Disk, j,
			base, len
			);
		j ++;
		
	}
	// Scan extended partitions
	while(extendedLBA != 0)
	{
		extendedLBA = ATA_MBR_int_ReadExt(Disk, extendedLBA, &base, &len);
		if(extendedLBA == -1)	break;
		ATA_int_MakePartition(
			&gATA_Disks[Disk].Partitions[j], Disk, k, base, len
			);
	}
	
	LEAVE('-');
}

/**
 * \brief Reads an extended partition
 * \return LBA of next Extended, -1 on error, 0 for last
 */
Uint64 ATA_MBR_int_ReadExt(int Disk, Uint64 Addr, Uint64 *Base, Uint64 *Length)
{
	Uint64	link = 0;
	 int	bFoundPart = 0;;
	 int	i;
	tMBR	mbr;
	Uint64	base, len;
	
	if( ATA_ReadDMA( Disk, Addr, 1, &mbr ) != 0 )
		return -1;	// Stop on Errors
	
	
	for( i = 0; i < 4; i ++ )
	{
		if( mbr.Parts[i].SystemID == 0 )	continue;
		
		// LBA 24
		if( mbr.Parts[i].Boot == 0x0 || mbr.Parts[i].Boot == 0x80 ) {
			base = mbr.Parts[i].LBAStart;
			len = mbr.Parts[i].LBALength;
		}
		// LBA 48
		else if( mbr.Parts[i].Boot == 0x1 || mbr.Parts[i].Boot == 0x81 ) {
			base = (mbr.Parts[i].StartHi << 16) | mbr.Parts[i].LBAStart;
			len = (mbr.Parts[i].LengthHi << 16) | mbr.Parts[i].LBALength;
		}
		else {
			Warning("Unknown partition type, Disk %i 0x%llx Part %i",
				Disk, Addr, i);
			return -1;
		}
		
		switch(mbr.Parts[i].SystemID)
		{
		case 0xF:
		case 0x5:
			if(link != 0) {
				Warning("Disk %i has two forward links in the extended partition",
					Disk);
				return -1;
			}
			link = base;
			break;
		default:
			if(bFoundPart) {
				Warning("Disk %i has more than one partition in the extended partition at 0x%llx",
					Disk, Addr);
				return -1;
			}
			bFoundPart = 1;
			*Base = base;
			*Length = len;
			break;
		}
	}
	
	if(!bFoundPart) {
		Warning("No partition in extended partiton, Disk %i 0x%llx",
			Disk, Addr);
		return -1;
	}
	
	return link;
}
