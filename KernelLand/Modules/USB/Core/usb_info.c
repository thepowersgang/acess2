/*
 * Acess 2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb_info.c
 * - USB Device Information Functions (helpers for drivers)
 */
#include <usb_core.h>
#include "usb.h"
#include "usb_lowlevel.h"

// === CODE ===
Uint32 USB_GetInterfaceClass(tUSBInterface *Dev)
{
	return ((Uint32)Dev->IfaceDesc.InterfaceClass << 16)
		|((Uint32)Dev->IfaceDesc.InterfaceSubClass << 8)
		|((Uint32)Dev->IfaceDesc.InterfaceProtocol << 0);
}

void USB_GetDeviceVendor(tUSBInterface *Dev, Uint16 *VendorID, Uint16 *DeviceID)
{
	*VendorID = LittleEndian16( Dev->Dev->DevDesc.VendorID );
	*DeviceID = LittleEndian16( Dev->Dev->DevDesc.DeviceID );
}
char *USB_GetSerialNumber(tUSBInterface *Dev)
{
	return USB_int_GetDeviceString(Dev->Dev, 0, Dev->Dev->DevDesc.SerialNumberStr);
}

