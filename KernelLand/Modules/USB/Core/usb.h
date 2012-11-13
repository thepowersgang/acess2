/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 * 
 * usb.h
 * - USB Internal definitions
 */
#ifndef _USB_H_
#define _USB_H_

#include <usb_core.h>
#include <usb_hub.h>
#include <usb_host.h>
#include "usb_proto.h"

typedef struct sUSBHubPort	tUSBHubPort;
typedef struct sUSBHost	tUSBHost;
typedef struct sUSBDevice	tUSBDevice;
typedef struct sUSBEndpoint	tUSBEndpoint;

// === STRUCTURES ===
struct sUSBHubPort
{
	void	*ListNext;
	char	Status;
	char	PortNum;
	tUSBDevice	*Dev;
};

/**
 * \brief USB Hub data
 */
struct sUSBHub
{
	tUSBInterface	*Interface;
	
	 int	nPorts;
	struct sUSBHubPort	Ports[];
};

struct sUSBEndpoint
{
	tUSBEndpoint	*Next;	// (usb_poll.c) Clock list
	tUSBInterface	*Interface;
	 int	EndpointIdx;	// Interface endpoint index
	 int	EndpointNum;	// Device endpoint num
	void	*EndpointHandle;
	
	 int	PollingPeriod;	// In 1ms intervals
	 int	MaxPacketSize;	// In bytes
	Uint8	Type;	// Same as sUSBDriver.Endpoints.Type
	
	 int	PollingAtoms;	// (usb_poll.c) Period in clock list
	void	*InputData;
};

/**
 * \brief Structure for a device's interface
 */
struct sUSBInterface
{
//	tUSBInterface	*Next;
	tUSBDevice	*Dev;

	tUSBDriver	*Driver;
	void	*Data;

	struct sDescriptor_Interface	IfaceDesc;
	
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

	void	*EndpointHandles[16];

	struct sDescriptor_Device	DevDesc;

	 int	nInterfaces;
	tUSBInterface	*Interfaces[];
};

struct sUSBHost
{
	struct sUSBHost	*Next;
	
	tUSBHostDef	*HostDef;
	void	*Ptr;
	
	Uint8	AddressBitmap[128/8];
	
	tUSBDevice	*RootHubDev;
	tUSBInterface	*RootHubIf;
	tUSBHub	RootHub;
};

extern tUSBDriver	*USB_int_FindDriverByClass(Uint32 ClassCode);

#endif
