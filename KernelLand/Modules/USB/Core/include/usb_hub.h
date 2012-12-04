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

/**
 * \brief Register a device as a hub
 * 
 * Used by the hub class initialisation routine.
 */
extern tUSBHub	*USB_RegisterHub(tUSBInterface *Device, int nPorts);
extern void	USB_RemoveHub(tUSBHub *Hub);

extern void	USB_DeviceConnected(tUSBHub *Hub, int Port);
extern void	USB_DeviceDisconnected(tUSBHub *Hub, int Port);

#define PORT_CONNECTION	0
#define PORT_ENABLE	1
#define PORT_SUSPEND	2
#define PORT_OVER_CURRENT	3
#define PORT_RESET	4
#define PORT_POWER	8
#define PORT_LOW_SPEED	9
#define C_PORT_CONNECTION	16
#define C_PORT_ENABLE	17
#define C_PORT_SUSPEND	18
#define C_PORT_OVER_CURRENT	19
#define C_PORT_RESET	20
#define PORT_TEST	21
#define PORT_INDICATOR	21

extern void	Hub_SetPortFeature(tUSBInterface *HubDev, int Port, int Feat);
extern void	Hub_ClearPortFeature(tUSBInterface *HubDev, int Port, int Feat);
extern int	Hub_GetPortStatus(tUSBInterface *HubDev, int Port, int Flag);

extern void	USB_PortCtl_BeginReset(tUSBHub *Hub, int Port);

#endif

