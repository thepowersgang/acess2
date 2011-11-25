/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb_host.h
 * - USB Host Controller Interface
 */
#ifndef _USB_HOST_H_
#define _USB_HOST_H_

#include "usb_core.h"
#include "usb_hub.h"

typedef struct sUSBHostDef	tUSBHostDef;

/**
 * \brief Defines a USB Host Controller type
 */
struct sUSBHostDef
{
	void	*(*SendIN)(void *Ptr, int Fcn, int Endpt, int DataTgl, int bIOC, void *Data, size_t Length);
	void	*(*SendOUT)(void *Ptr, int Fcn, int Endpt, int DataTgl, int bIOC, void *Data, size_t Length);
	void	*(*SendSETUP)(void *Ptr, int Fcn, int Endpt, int DataTgl, int bIOC, void *Data, size_t Length);
	
	void	(*CheckPorts)(void *Ptr);
};

extern tUSBHub	*USB_RegisterHost(tUSBHostDef *HostDef, void *ControllerPtr, int nPorts);

#endif

