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
typedef struct sUSBDevice	tUSBDevice;
typedef struct sUSBEndpoint	tUSBEndpoint;

// === STRUCTURES ===
/**
 * \brief USB Hub data
 */
struct sUSBHub
{
	tUSBInterface	*Interface;
	
	 int	nPorts;
	tUSBDevice	*Devices[];
};

struct sUSBEndpoint
{
	tUSBEndpoint	*Next;	// In the poll list
	tUSBInterface	*Interface;
	 int	EndpointNum;
	
	 int	PollingPeriod;	// In 1ms intervals
	 int	MaxPacketSize;	// In bytes

	Uint8	Type;	// Same as sUSBDriver.Endpoints.Type
};

/**
 * \brief Structure for a device's interface
 */
struct sUSBInterface
{
	tUSBInterface	*Next;
	tUSBDevice	*Dev;

	tUSBDriver	*Driver;
	void	*Data;
	
	 int	nEndpoints;
	tUSBEndpoint	Endpoints[];
};

/**
 * \brief Defines a single device on the USB Bus
 */
struct sUSBDevice
{
	tUSBHub	*ParentHub;

	/**
	 * \brief Host controller used
	 */
	tUSBHost	*Host;
	 int	Address;

	 int	nInterfaces;
	tUSBInterface	*Interfaces[];
};

struct sUSBHost
{
	struct sUSBHost	*Next;
	
	tUSBHostDef	*HostDef;
	void	*Ptr;
	
	Uint8	AddressBitmap[128/8];
	
	tUSBDevice	RootHubDev;
	tUSBInterface	RootHubIf;
	tUSBHub	RootHub;
};

extern void	USB_NewDevice(tUSBHub *Hub);

#endif
