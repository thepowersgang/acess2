/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * include/udi_pci.h
 * - PCI Bus Binding
 */
#ifndef _UDI_PCI_H_
#define _UDI_PCI_H_

#ifndef _UDI_PHYSIO_H_
# error "udi_pci.h requires udi_physio.h"
#endif

#define UDI_PCI_CONFIG_SPACE	255
#define UDI_PCI_BAR_0	0
#define UDI_PCI_BAR_1	1
#define UDI_PCI_BAR_2	2
#define UDI_PCI_BAR_3	3
#define UDI_PCI_BAR_4	4
#define UDI_PCI_BAR_5	5

#endif

