/*
 * Acess2 Logical Volume Manager
 * - By John Hodge (thePowersGang)
 * 
 * mbr.c
 * - MBR Parsing Code
 */
#define DEBUG	0
#include <acess.h>
#include "lvm.h"
#include "mbr.h"

// === PROTOTYPES ===
 int	LVM_MBR_Detect(tLVM_Vol *Volume);
 int	LVM_MBR_CountSubvolumes(tLVM_Vol *Volume, void *FirstSector);
void	LVM_MBR_PopulateSubvolumes(tLVM_Vol *Volume, void *FirstSector);
Uint64	LVM_MBR_int_ReadExt(tLVM_Vol *Volume, Uint64 Addr, Uint64 *Base, Uint64 *Length);

// === GLOBALS ===
tLVM_Format	gLVM_MBRType = {
	.Name = "MBR",
	.CountSubvolumes = LVM_MBR_CountSubvolumes,
	.PopulateSubvolumes = LVM_MBR_PopulateSubvolumes
};

// === CODE ===
int LVM_MBR_Detect(tLVM_Vol *Volume)
{
	tMBR	mbr;
	// TODO: handle non-512 byte sectors
	if( LVM_int_ReadVolume( Volume, 0, 1, &mbr ) != 0 )
		return -1;	// Stop on Errors

	if( mbr.BootFlag != LittleEndian16(0xAA55) )
		return 0;

	return 1;
}

/**
 * \brief Initialise a volume as 
 */
int LVM_MBR_CountSubvolumes(tLVM_Vol *Volume, void *FirstSector)
{
	tMBR	*MBR = FirstSector;
	Uint64	extendedLBA;
	Uint64	base, len;
	 int	numPartitions = 0;
	
	ENTER("pVolume pFirstSector", Volume, FirstSector);
	
	// Count Partitions
	numPartitions = 0;
	extendedLBA = 0;
	for( int i = 0; i < 4; i ++ )
	{
		if( MBR->Parts[i].SystemID == 0 )	continue;
	
		if( MBR->Parts[i].Boot == 0x0 || MBR->Parts[i].Boot == 0x80 )	// LBA 28
		{
			base = MBR->Parts[i].LBAStart;
		}
		else if( MBR->Parts[i].Boot == 0x1 || MBR->Parts[i].Boot == 0x81 )	// LBA 48
		{
			base = (MBR->Parts[i].StartHi << 16) | MBR->Parts[i].LBAStart;
		}
		else
			continue ;	// Invalid, so don't count
		
		if( MBR->Parts[i].SystemID == 0xF || MBR->Parts[i].SystemID == 5 )
		{
			LOG("Extended Partition at 0x%llx", base);
			if(extendedLBA != 0) {
				Log_Warning(
					"LBA MBR",
					"Volume %p has multiple extended partitions, ignoring all but first",
					Volume
					);
				continue;
			}
			extendedLBA = base;
		}
		else
		{
			LOG("Primary Partition at 0x%llx", base);
			numPartitions ++;
		}
	}
	// Detect the GPT protector
	if( extendedLBA == 0 && numPartitions == 1 && MBR->Parts[0].SystemID == 0xEE )
	{
		// TODO: Hand off to GPT parsing code
		Log_Warning("LBA MBR", "TODO: Hand off to GPT");
	}
	
	// Handle extended partions
	while(extendedLBA != 0)
	{
		extendedLBA = LVM_MBR_int_ReadExt(Volume, extendedLBA, &base, &len);
		if( extendedLBA == (Uint64)-1 )
			break;
		numPartitions ++;
	}
	LOG("numPartitions = %i", numPartitions);

	LEAVE('i', numPartitions);
	return numPartitions;	
}

void LVM_MBR_PopulateSubvolumes(tLVM_Vol *Volume, void *FirstSector)
{
	Uint64	extendedLBA;
	Uint64	base, len;
	 int	i, j;
	tMBR	*MBR = FirstSector;

	ENTER("pVolume pFirstSector", Volume, FirstSector);
	
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
		else if( MBR->Parts[i].Boot == 0x1 || MBR->Parts[i].Boot == 0x81 )	// LBA 48
		{
			base = (MBR->Parts[i].StartHi << 16) | MBR->Parts[i].LBAStart;
			len = (MBR->Parts[i].LengthHi << 16) | MBR->Parts[i].LBALength;
		}
		else
			continue;
		
		if( MBR->Parts[i].SystemID == 0xF || MBR->Parts[i].SystemID == 5 ) {
			if(extendedLBA == 0)
				extendedLBA = base;
			continue;
		}
		// Create Partition
		LVM_int_SetSubvolume_Anon( Volume, j, base, len );
		j ++;
		
	}
	// Scan extended partitions
	while(extendedLBA != 0)
	{
		extendedLBA = LVM_MBR_int_ReadExt(Volume, extendedLBA, &base, &len);
		if(extendedLBA == (Uint64)-1)
			break;
		LVM_int_SetSubvolume_Anon( Volume, j, base, len );
		j ++ ;
	}
	
	LEAVE('-');
}

/**
 * \brief Reads an extended partition
 * \return LBA of next Extended, -1 on error, 0 for last
 */
Uint64 LVM_MBR_int_ReadExt(tLVM_Vol *Volume, Uint64 Addr, Uint64 *Base, Uint64 *Length)
{
	Uint64	link = 0;
	 int	bFoundPart = 0;;
	 int	i;
	tMBR	mbr;
	Uint64	base, len;
	
	// TODO: Handle non-512 byte sectors
	if( LVM_int_ReadVolume( Volume, Addr, 1, &mbr ) != 0 )
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
			Log_Warning("LVM MBR",
				"Unknown partition type 0x%x, Volume %p Ext 0x%llx Part %i",
				mbr.Parts[i].Boot, Volume, Addr, i
				);
			return -1;
		}
		
		switch(mbr.Parts[i].SystemID)
		{
		case 0xF:
		case 0x5:
			if(link != 0) {
				Log_Warning("LVM MBR",
					"Volume %p has two forward links in the extended partition",
					Volume
					);
				return -1;
			}
			link = base;
			break;
		default:
			if(bFoundPart) {
				Warning("LVM MBR",
					"Volume %p has more than one partition in the extended partition at 0x%llx",
					Volume, Addr
					);
				return -1;
			}
			bFoundPart = 1;
			*Base = Addr + base;	// Extended partitions are based off the sub-mbr
			*Length = len;
			break;
		}
	}
	
	if(!bFoundPart) {
		Log_Warning("LVM MBR",
			"No partition in extended partiton, Volume %p 0x%llx",
			Volume, Addr
			);
		return -1;
	}
	
	return link;
}
