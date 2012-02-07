/*
 * Acess2 82077AA FDC
 * - By John Hodge (thePowersGang)
 *
 * common.h
 * - Common definitions
 */
#ifndef _FDC_COMMON_H_
#define _FDC_COMMON_H_

#include <mutex.h>
#include <timers.h>

// === CONSTANTS ===
#define MAX_DISKS	8	// 4 per controller, 2 controllers
#define TRACKS_PER_DISK	(1440*2/18)
#define BYTES_PER_TRACK	(18*512)

// === TYPEDEFS ===
typedef struct sFDD_Drive	tDrive;

// === STRUCTURES ===
struct sFDD_Drive
{
	 int	bValid;
	 int	bInserted;
	 int	MotorState;
	tTimer	*Timer;

	tMutex	Mutex;
	
	void	*TrackData[TRACKS_PER_DISK];	// Whole tracks are read
};

// === FUNCTIONS ===
extern int	FDD_SetupIO(void);
extern int	FDD_int_ReadWriteTrack(int Disk, int Track, int bWrite, void *Buffer);

// === GLOBALS ===
extern tDrive	gaFDD_Disks[MAX_DISKS];

#endif

