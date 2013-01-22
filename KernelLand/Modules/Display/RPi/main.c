/**
 * Acess2 Raspberry Pi (BCM2835)
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Driver core
 *
 * http://www.cl.cam.ac.uk/freshers/raspberrypi/tutorials/os/screen01.html
 */
#define DEBUG	0
#define VERSION	((0<<8)|1)
#include <acess.h>
#include <errno.h>
#include <modules.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <api_drv_video.h>

// === PROTOCOLS ===
 int	RPiVid_Install(const char **Arguments);
// - GPU Communication
 int	RPiVid_int_MBoxCheck(Uint8 Box);
Uint32	RPiVid_int_MBoxRecv(Uint8 Box);
void	RPiVid_int_MBoxSend(Uint8 Box, Uint32 Message);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, RPiVid, RPiVid_Install, NULL, NULL);

// === CODE ===
int RPiVid_Install(const char **Arguments)
{
	return 0;
}


// --- GPU Comms ---
int RPiVid_int_MBoxCheck(Uint8 Box)
{
	return 0;
}

Uint32 RPiVid_int_MBoxRecv(Uint8 Box)
{
	Uint32	val;
	do {
		while( gRPiVid_Mbox->Status & (1 << 30) )
			;
		val = gRPiVid_Mbox->Read;
	} while( (val & 0xF) != Box );
	
	return 0;
}

void RPiVid_int_MBoxSend(Uint8 Box, Uint32 Message)
{
	while( gRPiVid_Mbox.Status & (1 << 31) )
		;
	gRPiVid_Mbox.Write = (Message << 4) | (Box & 0xF);
}

