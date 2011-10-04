/*
 * Acess 2 USB Stack
 * USB Packet Control
 */
#define DEBUG	1
#include <acess.h>
#include <vfs.h>
#include <drv_pci.h>
#include "usb.h"


// === CODE ===
void USB_RegisterHost(tUSBHost *HostDef, void *ControllerPtr)
{
	// TODO:
}

int USB_int_SendSetupSetAddress(tUSBHost *Host, void *Ptr, int Address)
{
	Uint8	data[8];
	data[0] = 0;	// bmRequestType
	data[1] = 5;	// SET_ADDRESS
	data[2] = Address & 0x7F;	// wValue (low)
	data[3] = 0;	// wValue (high)
	data[4] = 0;	// wLength
	data[6] = 0;	// wLength
	
	// Addr 0:0, Data Toggle = 0, no interrupt
	return Host->SendSETUP(Ptr, 0, 0, 0, FALSE, data, 8) == NULL;
}
