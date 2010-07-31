/*
 * Acess2 IDE Harddisk Driver
 * - main.c
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <acess.h>
#include <vfs.h>

// === CONSTANTS ===
#define	MAX_ATA_DISKS	4
#define	SECTOR_SIZE		512
#define	MAX_DMA_SECTORS	(0x1000 / SECTOR_SIZE)

#define	IDE_PRI_BASE	0x1F0
#define	IDE_SEC_BASE	0x170

#define	IDE_PRDT_LAST	0x8000
/**
 \enum HddControls
 \brief Commands to be sent to HDD_CMD
*/
enum HddControls {
	HDD_PIO_R28 = 0x20,
	HDD_PIO_R48 = 0x24,
	HDD_DMA_R48 = 0x25,
	HDD_PIO_W28 = 0x30,
	HDD_PIO_W48 = 0x34,
	HDD_DMA_W48 = 0x35,
	HDD_DMA_R28 = 0xC8,
	HDD_DMA_W28 = 0xCA,
};

// === STRUCTURES ===
typedef struct
{
	Uint8	BootCode[0x1BE];
	struct {
		Uint8	Boot;
		Uint8	Unused1;	// Also CHS Start
		Uint16	StartHi;	// Also CHS Start
		Uint8	SystemID;
		Uint8	Unused2;	// Also CHS Length
		Uint16	LengthHi;	// Also CHS Length
		Uint32	LBAStart;
		Uint32	LBALength;
	} __attribute__ ((packed)) Parts[4];
	Uint16	BootFlag;	// = 0xAA 55
} __attribute__ ((packed))	tMBR;

typedef struct
{
	Uint64	Start;
	Uint64	Length;
	char	Name[4];
	tVFS_Node	Node;
}	tATA_Partition;

typedef struct
{
	Uint64	Sectors;
	char	Name[2];
	tVFS_Node	Node;
	 int	NumPartitions;
	tATA_Partition	*Partitions;
}	tATA_Disk;

// === GLOBALS ===
extern tATA_Disk	gATA_Disks[];

// === FUNCTIONS ===
// --- Common ---
extern void	ATA_int_MakePartition(tATA_Partition *Part, int Disk, int Num, Uint64 Start, Uint64 Length);

// --- MBR Parsing ---
extern void	ATA_ParseMBR(int Disk, tMBR *MBR);

// --- IO Functions ---
extern int	ATA_SetupIO(void);
extern Uint64	ATA_GetDiskSize(int Disk);
extern int	ATA_ReadDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer);
extern int	ATA_WriteDMA(Uint8 Disk, Uint64 Address, Uint Count, const void *Buffer);

#endif
