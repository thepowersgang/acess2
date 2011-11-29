/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb.c
 * - USB Structure
 */
#define DEBUG	1
#include <acess.h>
#include <vfs.h>
#include <drv_pci.h>
#include "usb.h"

// === IMPORTS ===
extern tUSBHost	*gUSB_Hosts;
extern tUSBDriver gUSBHub_Driver;

// === STRUCTURES ===

// === PROTOTYPES ===
tUSBHub	*USB_RegisterHost(tUSBHostDef *HostDef, void *ControllerPtr, int nPorts);

// === GLOBALS ===
tUSBDriver	*gUSB_InterfaceDrivers = &gUSBHub_Driver;

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

	host->RootHubDev.ParentHub = NULL;
	host->RootHubDev.Host = host;
	host->RootHubDev.Address = 0;

	host->RootHubIf.Next = NULL;
	host->RootHubIf.Dev = &host->RootHubDev;
	host->RootHubIf.Driver = NULL;
	host->RootHubIf.Data = NULL;
	host->RootHubIf.nEndpoints = 0;

	host->RootHub.Interface = &host->RootHubIf;
	host->RootHub.nPorts = nPorts;
	memset(host->RootHub.Devices, 0, sizeof(void*)*nPorts);

	// TODO: Lock
	host->Next = gUSB_Hosts;
	gUSB_Hosts = host;

	return &host->RootHub;
}

void USB_RegisterDriver(tUSBDriver *Driver)
{
	Log_Warning("USB", "TODO: Implement USB_RegisterDriver");
}

// --- Hub Registration ---
// NOTE: Doesn't do much nowdays
tUSBHub *USB_RegisterHub(tUSBInterface *Device, int PortCount)
{
	tUSBHub	*ret;
	
	ret = malloc(sizeof(tUSBHub) + sizeof(ret->Devices[0])*PortCount);
	ret->Interface = Device;
	ret->nPorts = PortCount;
	memset(ret->Devices, 0, sizeof(ret->Devices[0])*PortCount);
	return ret;
}

void USB_RemoveHub(tUSBHub *Hub)
{
	for( int i = 0; i < Hub->nPorts; i ++ )
	{
		if( Hub->Devices[i] )
		{
			USB_DeviceDisconnected( Hub, i );
		}
	}
	free(Hub);
}

