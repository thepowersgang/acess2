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
tMutex	glUSB_Hosts;
tUSBHost	*gUSB_Hosts = NULL;
tMutex	glUSB_InterfaceDrivers;
tUSBDriver	*gpUSB_InterfaceDrivers = &gUSBHub_Driver;

// === CODE ===
tUSBHub *USB_RegisterHost(tUSBHostDef *HostDef, void *ControllerPtr, int nPorts)
{
	tUSBHost	*host;
	
	host = malloc(sizeof(tUSBHost) + nPorts*sizeof(tUSBHubPort) + sizeof(tUSBDevice) + sizeof(tUSBInterface));
	if(!host) {
		// Oh, bugger.
		return NULL;
	}
	host->HostDef = HostDef;
	host->Ptr = ControllerPtr;
	memset(host->AddressBitmap, 0, sizeof(host->AddressBitmap));

	host->RootHubDev = (void*)(host->RootHub.Ports + nPorts);
	host->RootHubDev->ParentHub = NULL;
	host->RootHubDev->Host = host;
	host->RootHubDev->Address = 0;
	ASSERT(HostDef->InitControl);
	host->RootHubDev->EndpointHandles[0] = HostDef->InitControl(ControllerPtr, 0, 64);
	host->RootHubDev->nInterfaces = 0;

	host->RootHubIf = (void*)(host->RootHubDev + 1);
	host->RootHubIf->Dev = host->RootHubDev;
	host->RootHubIf->Driver = NULL;
	host->RootHubIf->Data = NULL;
	host->RootHubIf->nEndpoints = 0;

	host->RootHub.Interface = host->RootHubIf;
	host->RootHub.nPorts = nPorts;
	memset(host->RootHub.Ports, 0, sizeof(tUSBHubPort)*nPorts);

	// Append to list
	Mutex_Acquire( &glUSB_Hosts );
	host->Next = gUSB_Hosts;
	gUSB_Hosts = host;
	Mutex_Release( &glUSB_Hosts );

	return &host->RootHub;
}

// --- Drivers ---
void USB_RegisterDriver(tUSBDriver *Driver)
{
	Mutex_Acquire( &glUSB_InterfaceDrivers );
	Driver->Next = gpUSB_InterfaceDrivers;
	gpUSB_InterfaceDrivers = Driver;
	Mutex_Release( &glUSB_InterfaceDrivers );
	
	// TODO: Recheck devices that didn't have a driver
}

tUSBDriver *USB_int_FindDriverByClass(Uint32 ClassCode)
{
	ENTER("xClassCode", ClassCode);
	for( tUSBDriver *ret = gpUSB_InterfaceDrivers; ret; ret = ret->Next )
	{
		LOG(" 0x%x & 0x%x == 0x%x?", ClassCode, ret->Match.Class.ClassMask, ret->Match.Class.ClassCode);
		if( (ClassCode & ret->Match.Class.ClassMask) == ret->Match.Class.ClassCode )
		{
			LOG("Found '%s'", ret->Name);
			LEAVE('p', ret);
			return ret;
		}
	}
	LEAVE('n');
	return NULL;
}

// --- Hub Registration ---
// NOTE: Doesn't do much nowdays
tUSBHub *USB_RegisterHub(tUSBInterface *Device, int PortCount)
{
	tUSBHub	*ret;
	
	ret = malloc(sizeof(tUSBHub) + sizeof(ret->Ports[0])*PortCount);
	ret->Interface = Device;
	ret->nPorts = PortCount;
	memset(ret->Ports, 0, sizeof(ret->Ports[0])*PortCount);
	return ret;
}

void USB_RemoveHub(tUSBHub *Hub)
{
	for( int i = 0; i < Hub->nPorts; i ++ )
	{
		if( Hub->Ports[i].Dev )
		{
			USB_DeviceDisconnected( Hub, i );
		}
	}
	free(Hub);
}

