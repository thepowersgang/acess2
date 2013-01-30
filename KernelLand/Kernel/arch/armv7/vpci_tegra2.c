/*
 * Acess2 Kernel ARMv7 Port
 * - By John Hodge (thePowersGang)
 *
 * vpci_tegra2.c
 * - Tegra2 VPCI Definitions
 */
#include <virtual_pci.h>

// === PROTOTYPES ===

// === GLOBALS ===
tVPCI_Device	gaVPCI_Devices[] = {
	#if 0
	{
	.Vendor=0x0ACE,.Device=0x1100,
	.Class = 0x0C032000,	// Serial, USB, ECHI
	.BARs = {0xC5000000,0,0,0,0,0},
	.IRQ = 0*32+20,
	},
	{
	.Vendor=0x0ACE,.Device=0x1100,
	.Class = 0x0C032000,	// Serial, USB, ECHI
	.BARs = {0xC5004000,0,0,0,0,0},
	.IRQ = 0*32+21,
	},
	{
	.Vendor=0x0ACE,.Device=0x1100,
	.Class = 0x0C032000,	// Serial, USB, ECHI
	.BARs = {0xC5008000,0,0,0,0,0},
	.IRQ = 4*32+1,
	}
	#endif
};
int giVPCI_DeviceCount = sizeof(gaVPCI_Devices)/sizeof(gaVPCI_Devices[0]);

