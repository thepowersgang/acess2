/*
 * Acess2 Kernel ARMv7 Port
 * - By John Hodge (thePowersGang)
 *
 * vpci_realview_pb.c
 * - Realview PB VPCI Definitions
 */
#include <virtual_pci.h>

// === PROTOTYPES ===

// === GLOBALS ===
tVPCI_Device	gaVPCI_Devices[] = {
};
int giVPCI_DeviceCount = sizeof(gaVPCI_Devices)/sizeof(gaVPCI_Devices[0]);

