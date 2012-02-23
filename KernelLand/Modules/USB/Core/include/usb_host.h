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

typedef void	(*tUSBHostCb)(void *DataPtr, void *Data, int Length);

typedef void	*(*tUSBHostOp)(void *Ptr, int Dest, int DataTgl, tUSBHostCb CB, void *CbData, void *Data, size_t Length);

/**
 * \brief Defines a USB Host Controller type
 */
struct sUSBHostDef
{
	tUSBHostOp	SendIN;
	tUSBHostOp	SendOUT;
	tUSBHostOp	SendSETUP;

	/**
	 * \brief Check if an operation has completed
	 * \note Only valid to call if CB passed was ERRPTR
	 */
	 int	(*IsOpComplete)(void *Ptr, void *OpPtr);

	void	(*CheckPorts)(void *Ptr);
};

extern tUSBHub	*USB_RegisterHost(tUSBHostDef *HostDef, void *ControllerPtr, int nPorts);

#endif

