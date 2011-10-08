/*
 * Acess2 82077AA FDC
 * - By John Hodge (thePowersGang)
 *
 * common.h
 * - Common definitions
 */
#ifndef _FDC_COMMON_H_
#define _FDC_COMMON_H_

// === CONSTANTS ===
#define MAX_DISKS	8	// 4 per controller, 2 controllers
#define TRACKS_PER_DISK	(1440*2/18)
#define BYTES_PER_TRACK	(18*512)

// === TYPEDEFS ===
typedef struct sFDD_Drive	tDrive;

// === STRUCTURES ===
struct sFDD_Drive
{
	 int	bInserted;
	 int	MotorState;
	 int	Timer;
	
	void	*TrackData[TRACKS_PER_DISK];	// Whole tracks are read
};

// === FUNCTIONS ===
extern void	FDD_int_IRQHandler(int IRQ, void *Ptr);

// === GLOBALS ===
extern tDrive	gaFDD_Disks[MAX_DISKS];

#endif

