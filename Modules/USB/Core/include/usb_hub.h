/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb_hub.h
 * - Core Hub Definitions
 */
#ifndef _USB_HUB_H_
#define _USB_HUB_H_

#include "usb_core.h"

typedef struct sUSBHub	tUSBHub;

typedef void	(*tUSB_HubPoll)(tUSBHub *Hub, tUSBDevice *HubDev);

/**
 * \brief Register a device as a hub
 * 
 * Used by the hub class initialisation routine.
 */
extern tUSBHub	USB_RegisterHub(tUSBDevice *Device, int nPorts, tUSB_HubPoll PollCallback);

extern void	USB_DeviceConnected(tUSBHub *Hub, int Port);
extern void	USB_DeviceDisconnected(tUSBHub *Hub, int Port);

#endif

