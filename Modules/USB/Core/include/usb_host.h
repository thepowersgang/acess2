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

typedef void	(*tUSBHostCb)(void *DataPtr);

typedef void	*(*tUSBHostOp)(void *Ptr, int Fcn, int Endpt, int DataTgl, tUSBHostCb bIOC, void *Data, size_t Length);

/**
 * \brief Defines a USB Host Controller type
 */
struct sUSBHostDef
{
	tUSBHostOp	SendIN;
	tUSBHostOp	SendOUT;
	tUSBHostOp	SendSETUP;

	 int	(*IsOpComplete)(void *Ptr, void *OpPtr);

	void	(*CheckPorts)(void *Ptr);
};

extern tUSBHub	*USB_RegisterHost(tUSBHostDef *HostDef, void *ControllerPtr, int nPorts);

#endif

