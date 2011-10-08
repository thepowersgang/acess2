/*
 * Acess2 82077AA FDC
 * - By John Hodge (thePowersGang)
 *
 * fdc.c
 * - Core file
 */
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include "common.h"

// === CONSTANTS ===

// === STRUCTURES ===

// === PROTOTYPES ===
 int	FDD_Install(char **Arguments);

// === GLOBALS ===
MODULE_DEFINE(0, 0x110, Storage_FDDv2, FDD_Install, NULL, "x86_ISADMA", NULL);
tDrive	gaFDD_Disks[MAX_DISKS];

// === CODE ===
int FDD_Install(char **Arguments)
{
	return 0;
}

