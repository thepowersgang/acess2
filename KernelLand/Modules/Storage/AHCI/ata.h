/*
 * Acess2
 * - By John Hodge (thePowersGang)
 *
 * ata.h
 * - ATA Definitions
 */
#ifndef _AHCI__ATA_H_
#define _AHCI__ATA_H_

enum eATA_Commands
{
	ATA_CMD_IDENTIFY_DEVICE = 0xEC,
	ATA_CMD_READDMA48	= 0x25,
	ATA_CMD_READDMA28	= 0xC8,
};

#define ATA_STATUS_ERR	0x01	// Error encountered
#define ATA_STATUS_DRQ	0x08	// Data Request
#define ATA_STATUS_CDEP	0x10	// Command-dependent
#define ATA_STATUS_DF	0x20	// Device Fault
#define ATA_STATUS_DRDY	0x40	// Device ready 
#define ATA_STATUS_BSY	0x80	// Device busy

typedef struct sATA_Identify tATA_Identify;

/**
 * \brief Structure returned by the ATA IDENTIFY command
 */
struct sATA_Identify
{
	Uint16	Flags;		// 1
	Uint16	Usused1[9];	// 10
	char	SerialNum[20];	// 20
	Uint16	Usused2[3];	// 23
	char	FirmwareVer[8];	// 27
	char	ModelNumber[40];	// 47
	Uint16	SectPerInt;	// 48 - Low byte only
	Uint16	Unused3;	// 49
	Uint16	Capabilities[2];	// 51
	Uint16	Unused4[2];	// 53
	Uint16	ValidExtData;	// 54
	Uint16	Unused5[5];	 // 59
	Uint16	SizeOfRWMultiple;	// 60
	Uint32	Sectors28;	// LBA 28 Sector Count
	Uint16	Unused6[100-62];
	Uint64	Sectors48;	// LBA 48 Sector Count
	Uint16	Unused7[256-104];
} PACKED;

#endif

