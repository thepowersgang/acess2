/*
 * Acess2 IDE Harddisk Driver
 * - MBR Parsing Code
 * mbr.c
 */
#define DEBUG	0
#include <acess.h>
#include "common.h"

// == GLOBALS ===

// === CODE ===
/**
 * \fn void ATA_ParseMBR(int Disk)
 */
void ATA_ParseMBR(int Disk)
{
	 int	i, j = 0, k = 4;
	tMBR	mbr;
	Uint64	extendedLBA;
	
	ENTER("iDisk", Disk);
	
	// Read Boot Sector
	ATA_ReadDMA( Disk, 0, 1, &mbr );
	
	// Count Partitions
	gATA_Disks[Disk].NumPartitions = 0;
	extendedLBA = 0;
	for( i = 0; i < 4; i ++ )
	{
		if( mbr.Parts[i].SystemID == 0 )	continue;
		if(
			mbr.Parts[i].Boot == 0x0 || mbr.Parts[i].Boot == 0x80	// LBA 28
		||	mbr.Parts[i].Boot == 0x1 || mbr.Parts[i].Boot == 0x81	// LBA 48
			)
		{
			if( mbr.Parts[i].SystemID == 0xF || mbr.Parts[i].SystemID == 5 ) {
				LOG("Extended Partition");
				if(extendedLBA != 0) {
					Warning("Disk %i has multiple extended partitions, ignoring rest", Disk);
					continue;
				}
				extendedLBA = mbr.Parts[i].LBAStart;
				continue;
			}
			LOG("Primary Partition");
			
			gATA_Disks[Disk].NumPartitions ++;
			continue;
		}
		// Invalid Partition, so don't count it
	}
	while(extendedLBA != 0)
	{
		if( ATA_ReadDMA( Disk, extendedLBA, 1, &mbr ) != 0 )
			break;	// Stop on Errors
		
		extendedLBA = 0;
		
		if( mbr.Parts[0].SystemID == 0 )	continue;
		if(	mbr.Parts[0].Boot == 0x0 || mbr.Parts[0].Boot == 0x80	// LBA 28
		||	mbr.Parts[0].Boot == 0x1 || mbr.Parts[0].Boot == 0x81	// LBA 48
			)
		{
			if(mbr.Parts[0].SystemID == 0xF || mbr.Parts[0].SystemID == 0x7)
				extendedLBA = mbr.Parts[0].LBAStart;
			else
				gATA_Disks[Disk].NumPartitions ++;
		}
		
		if( mbr.Parts[1].SystemID == 0 )	continue;
		if(	mbr.Parts[1].Boot == 0x0 || mbr.Parts[1].Boot == 0x80	// LBA 28
		||	mbr.Parts[1].Boot == 0x1 || mbr.Parts[1].Boot == 0x81	// LBA 48
			)
		{
			if(mbr.Parts[1].SystemID == 0xF || mbr.Parts[1].SystemID == 0x7) {
				if(extendedLBA == 0) {
					Warning("Disk %i has twp forward link in the extended partition",
						Disk);
					break;
				}
				extendedLBA = mbr.Parts[1].LBAStart;
			}
			else {
				if(extendedLBA != 0) {
					Warning("Disk %i lacks a forward link in the extended partition",
						Disk);
					break;
				}
				gATA_Disks[Disk].NumPartitions ++;
			}
		}
	}
	LOG("gATA_Disks[Disk].NumPartitions = %i", gATA_Disks[Disk].NumPartitions);
	
	// Create patition array
	gATA_Disks[Disk].Partitions = malloc( gATA_Disks[Disk].NumPartitions * sizeof(tATA_Partition) );
	
	// --- Fill Partition Info ---
	extendedLBA = 0;
	for( i = 0; i < 4; i ++ )
	{
		Log("mbr.Parts[%i].SystemID = 0x%02x", i, mbr.Parts[i].SystemID);
		if( mbr.Parts[i].SystemID == 0 )	continue;
		if(	mbr.Parts[i].Boot == 0x0 || mbr.Parts[i].Boot == 0x80 )	// LBA 28
		{
			if( mbr.Parts[1].SystemID == 0xF || mbr.Parts[1].SystemID == 5 ) {
				if(extendedLBA != 0) {
					Warning("Disk %i has multiple extended partitions, ignoring rest", Disk);
					continue;
				}
				extendedLBA = mbr.Parts[1].LBAStart;
				continue;
			}
			// Create Partition
			ATA_int_MakePartition( &gATA_Disks[Disk].Partitions[j], Disk, i,
				mbr.Parts[i].LBAStart, mbr.Parts[i].LBALength
				);
			j ++;
			continue;
		}
		if(	mbr.Parts[i].Boot == 0x1 || mbr.Parts[i].Boot == 0x81 )	// LBA 48
		{
			if( mbr.Parts[i].SystemID == 0xF || mbr.Parts[i].SystemID == 5 ) {
				if(extendedLBA != 0) {
					Warning("Disk %i has multiple extended partitions, ignoring rest", Disk);
					continue;
				}
				extendedLBA = (mbr.Parts[i].StartHi << 16) | mbr.Parts[i].LBAStart;
				continue;
			}
			ATA_int_MakePartition( &gATA_Disks[Disk].Partitions[j], Disk, i,
				(mbr.Parts[i].StartHi << 16) | mbr.Parts[i].LBAStart,
				(mbr.Parts[i].LengthHi << 16) | mbr.Parts[i].LBALength
				);
			j ++;
		}
		// Invalid Partition, so don't count it
	}
	// Scan extended partition
	while(extendedLBA != 0)
	{
		if( ATA_ReadDMA( Disk, extendedLBA, 1, &mbr ) != 0 )
			break;	// Stop on Errors
		
		extendedLBA = 0;
		
		// Check first entry (should be partition)
		if( mbr.Parts[0].SystemID != 0)
		{
			if( mbr.Parts[0].Boot == 0x0 || mbr.Parts[0].Boot == 0x80 )	// LBA 28
			{
				// Forward Link to next Extended partition entry
				if(mbr.Parts[0].SystemID == 0xF || mbr.Parts[0].SystemID == 0x7)
					extendedLBA = mbr.Parts[0].LBAStart;
				else {
					ATA_int_MakePartition( &gATA_Disks[Disk].Partitions[j], Disk, k,
						mbr.Parts[0].LBAStart, mbr.Parts[0].LBALength
						);
					j ++;	k ++;
				}
			}
			else if( mbr.Parts[0].Boot == 0x1 || mbr.Parts[0].Boot == 0x81 )	// LBA 48
			{
				if(mbr.Parts[0].SystemID == 0xF || mbr.Parts[0].SystemID == 0x7)
					extendedLBA = (mbr.Parts[0].StartHi << 16) | mbr.Parts[0].LBAStart;
				else {
					ATA_int_MakePartition( &gATA_Disks[Disk].Partitions[j], Disk, k,
						(mbr.Parts[0].StartHi << 16) | mbr.Parts[0].LBAStart,
						(mbr.Parts[0].LengthHi << 16) | mbr.Parts[0].LBALength
						);
					j ++;	k ++;
				}
			}
		}
		
		// Check second entry (should be forward link)
		if( mbr.Parts[1].SystemID != 0)
		{
			if(mbr.Parts[1].Boot == 0x0 || mbr.Parts[1].Boot == 0x80 )	// LBA 28
			{
				if(mbr.Parts[1].SystemID == 0xF || mbr.Parts[1].SystemID == 0x7) {
					if(extendedLBA == 0) {
						Warning("Disk %i has twp forward link in the extended partition",
							Disk);
						break;
					}
					extendedLBA = mbr.Parts[1].LBAStart;
				}
				else
				{
					if(extendedLBA != 0) {
						Warning("Disk %i lacks a forward link in the extended partition",
							Disk);
						break;
					}
					ATA_int_MakePartition( &gATA_Disks[Disk].Partitions[j], Disk, k,
						mbr.Parts[1].LBAStart, mbr.Parts[1].LBALength
						);
					j ++;	k ++;
				}
				
			}
			else if( mbr.Parts[1].Boot == 0x1 || mbr.Parts[1].Boot == 0x81 )	// LBA 48
			{
				if(mbr.Parts[1].SystemID == 0xF || mbr.Parts[1].SystemID == 0x7) {
					if(extendedLBA == 0) {
						Warning("Disk %i has twp forward link in the extended partition",
							Disk);
						break;
					}
					extendedLBA = (mbr.Parts[1].StartHi << 16) | mbr.Parts[1].LBAStart;
				}
				else
				{
					if(extendedLBA != 0) {
						Warning("Disk %i lacks a forward link in the extended partition",
							Disk);
						break;
					}
					ATA_int_MakePartition( &gATA_Disks[Disk].Partitions[j], Disk, k,
						(mbr.Parts[1].StartHi << 16) | mbr.Parts[1].LBAStart,
						(mbr.Parts[1].LengthHi << 16) | mbr.Parts[1].LBALength
						);
					j ++;	k ++;
				}
			}
		}
	}
	
	LEAVE('-');
}
