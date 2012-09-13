/*
 * Acess2 Kernel x86 Port
 * - By John Hodge (thePowersGang)
 *
 * vpci.c
 * - Virtual PCI Bus
 */
#include <virtual_pci.h>

// === GLOBALS ===
 int	giVPCI_DeviceCount = 0;
tVPCI_Device	gaVPCI_Devices[0];

