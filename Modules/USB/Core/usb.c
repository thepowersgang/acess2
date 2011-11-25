/*
 * Acess 2 USB Stack
 * USB Packet Control
 */
#define DEBUG	1
#include <acess.h>
#include <vfs.h>
#include <drv_pci.h>
#include "usb.h"
#include "usb_proto.h"

// === STRUCTURES ===

// === CODE ===
tUSBHub *USB_RegisterHost(tUSBHostDef *HostDef, void *ControllerPtr, int nPorts)
{
	// TODO:
	return NULL;
}

void USB_DeviceConnected(tUSBHub *Hub, int Port)
{
	if( Port >= Hub->nPorts )	return ;
	if( Hub->Devices[Port] )	return ;

	// 0. Perform port init? (done in hub?)	
	// 1. Assign an address
	
	// 2. Get device information
}

void USB_DeviceDisconnected(tUSBHub *Hub, int Port)
{
	
}

void *USB_GetDeviceDataPtr(tUSBDevice *Dev) { return Dev->Data; }
void USB_SetDeviceDataPtr(tUSBDevice *Dev, void *Ptr) { Dev->Data = Ptr; }

int USB_int_AllocateAddress(tUSBHost *Host)
{
	 int	i;
	for( i = 1; i < 128; i ++ )
	{
		if(Host->AddressBitmap[i/8] & (1 << i))
			continue ;
		return i;
	}
	return 0;
}

int USB_int_SendSetupSetAddress(tUSBHost *Host, void *Ptr, int Address)
{
	struct sDeviceRequest	req;
	req.ReqType = 0;	// bmRequestType
	req.Request = 5;	// SET_ADDRESS
	req.Value = Address & 0x7F;	// wValue
	req.Index = 0;	// wIndex
	req.Length = 0;	// wLength
	
	// Addr 0:0, Data Toggle = 0, no interrupt
	return Host->HostDef->SendSETUP(Ptr, 0, 0, 0, FALSE, &req, sizeof(req)) == NULL;
}
