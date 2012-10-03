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
#define	SECTOR_SIZE 	512
#define	ATA_TIMEOUT 	2000	// 2s timeout
// Needed out of io.c because it's the max for Read/WriteDMA
#define	MAX_DMA_SECTORS	(0x1000 / SECTOR_SIZE)

// === FUNCTIONS ===
// --- IO Functions ---
extern int	ATA_SetupIO(void);
extern Uint64	ATA_GetDiskSize(int Disk);
extern int	ATA_ReadDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer);
extern int	ATA_WriteDMA(Uint8 Disk, Uint64 Address, Uint Count, const void *Buffer);

#endif
