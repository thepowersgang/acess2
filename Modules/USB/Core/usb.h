/*
 * AcessOS Version 1
 * USB Stack
 */
#ifndef _USB_H_
#define _USB_H_

#include <usb_core.h>
#include <usb_hub.h>
#include <usb_host.h>

typedef struct sUSBHost	tUSBHost;

// === STRUCTURES ===
/**
 * \brief USB Hub data
 */
struct sUSBHub
{
	tUSBDevice	*Device;
	
	tUSB_HubPoll	CheckPorts;
	
	 int	nPorts;
	tUSBDevice	*Devices[];
};

/**
 * \brief Defines a single device on the USB Bus
 */
struct sUSBDevice
{
	tUSBDevice	*Next;
	tUSBHub	*ParentHub;

	/**
	 * \brief Host controller used
	 */
	tUSBHost	*Host;
	 int	Address;
	
	tUSBDriver	*Driver;
	void	*Data;
};

struct sUSBHost
{
	struct sUSBHost	*Next;
	
	tUSBHostDef	*HostDef;
	void	*Ptr;
	
	Uint8	AddressBitmap[128/8];
	
	tUSBDevice	RootHubDev;
	tUSBHub	RootHub;
};

extern void	USB_NewDevice(tUSBHub *Hub);

#endif
