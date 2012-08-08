/*
 * Acess2 NVidia Graphics Driver
 * - By John Hodge (thePowersGang)
 * 
 * main.c
 * - Driver Core
 *
 * Reference: linux/drivers/video/nvidia
 */
#define DEBUG	1
#define VERSION VER2(0,1)
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include "regs.h"

// === PROTOTYPES ===
 int	NV_Install(char **Arguments);
 int	NV_Cleanup(void);

// === GLOBALS ===
MODULE_DEFINE(0, Video_NVidia, VERSION, NV_Install, NV_Cleanup, NULL);

// === CODE ===
int NV_Install(char **Arguments)
{
	return MODERR_NOTNEEDED;
}

int NV_Cleanup(void)
{
	return 0;
}

