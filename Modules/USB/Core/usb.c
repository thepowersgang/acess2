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

// === IMPORTS ===
extern tUSBHost	*gUSB_Hosts;

// === STRUCTURES ===

// === PROTOTYPES ===
tUSBHub	*USB_RegisterHost(tUSBHostDef *HostDef, void *ControllerPtr, int nPorts);
void	USB_DeviceConnected(tUSBHub *Hub, int Port);
void	USB_DeviceDisconnected(tUSBHub *Hub, int Port);
void	*USB_GetDeviceDataPtr(tUSBDevice *Dev);
void	USB_SetDeviceDataPtr(tUSBDevice *Dev, void *Ptr);
 int	USB_int_AllocateAddress(tUSBHost *Host);
 int	USB_int_SendSetupSetAddress(tUSBHost *Host, int Address);

// === CODE ===
tUSBHub *USB_RegisterHost(tUSBHostDef *HostDef, void *ControllerPtr, int nPorts)
{
	tUSBHost	*host;
	
	host = malloc(sizeof(tUSBHost) + nPorts*sizeof(void*));
	if(!host) {
		// Oh, bugger.
		return NULL;
	}
	host->HostDef = HostDef;
	host->Ptr = ControllerPtr;
	memset(host->AddressBitmap, 0, sizeof(host->AddressBitmap));

	host->RootHubDev.Next = NULL;
	host->RootHubDev.ParentHub = NULL;
	host->RootHubDev.Host = host;
	host->RootHubDev.Address = 0;
	host->RootHubDev.Driver = NULL;
	host->RootHubDev.Data = NULL;

	host->RootHub.Device = NULL;
	host->RootHub.CheckPorts = NULL;
	host->RootHub.nPorts = nPorts;
	memset(host->RootHub.Devices, 0, sizeof(void*)*nPorts);

	// TODO: Lock
	host->Next = gUSB_Hosts;
	gUSB_Hosts = host;

	// Initialise?
	HostDef->CheckPorts(ControllerPtr);
	
	return &host->RootHub;
}

void USB_DeviceConnected(tUSBHub *Hub, int Port)
{
	tUSBDevice	*dev;
	if( Port >= Hub->nPorts )	return ;
	if( Hub->Devices[Port] )	return ;

	ENTER("pHub iPort", Hub, Port);

	// 0. Perform port init? (done in hub?)	
	
	// Create structure
	dev = malloc(sizeof(tUSBDevice));
	dev->Next = NULL;
	dev->ParentHub = Hub;
	dev->Host = Hub->Device->Host;
	dev->Address = 0;
	dev->Driver = 0;
	dev->Data = 0;

	// 1. Assign an address
	dev->Address = USB_int_AllocateAddress(dev->Host);
	if(dev->Address == 0) {
		Log_Error("USB", "No addresses avaliable on host %p", dev->Host);
		free(dev);
		LEAVE('-');
		return ;
	}
	USB_int_SendSetupSetAddress(dev->Host, dev->Address);
	LOG("Assigned address %i", dev->Address);
	
	// 2. Get device information

	// Done.
	LEAVE('-');
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

int USB_int_SendSetupSetAddress(tUSBHost *Host, int Address)
{
	struct sDeviceRequest	req;
	req.ReqType = 0;	// bmRequestType
	req.Request = 5;	// SET_ADDRESS
	req.Value = Address & 0x7F;	// wValue
	req.Index = 0;	// wIndex
	req.Length = 0;	// wLength
	
	// Addr 0:0, Data Toggle = 0, no interrupt
	return Host->HostDef->SendSETUP(Host->Ptr, 0, 0, 0, FALSE, &req, sizeof(req)) == NULL;
}

int USB_int_ReadDescriptor(tUSBDevice *Dev, int Endpoint, int Type, int Index, int Length, void *Dest)
{
	struct sDeviceRequest	req;
	const int	ciMaxPacketSize = 0x400;
	req.ReqType = 0x80;
	req.Request = 6;	// GET_DESCRIPTOR
	req.Value = ((Type & 0xFF) << 8) | (Index & 0xFF);
	req.Index = 0;	// TODO: Language ID
	req.Length = Length;
	
	Dev->Host->HostDef->SendSETUP(
		Dev->Host->Ptr, Dev->Address, Endpoint,
		0, FALSE,
		&req, sizeof(req)
		);
	
	while( Length > ciMaxPacketSize )
	{
		Dev->Host->HostDef->SendIN(
			Dev->Host->Ptr, Dev->Address, Endpoint,
			1, FALSE,
			Dest, ciMaxPacketSize
			);
		Length -= ciMaxPacketSize;
	}

	// TODO: Complete and get completion	

	return 0;
}
