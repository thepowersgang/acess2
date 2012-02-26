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

typedef void	(*tUSBHostCb)(void *DataPtr, void *Data, size_t Length);

typedef void	*(*tUSBHostOp)(void *Ptr, int Dest, int DataTgl, tUSBHostCb CB, void *CbData, void *Data, size_t Length);
typedef void	*(*tUSBIntOp)(void *Ptr, int Dest, int Period, tUSBHostCb CB, void *CbData, void *Data, size_t Length);

/**
 * \brief Defines a USB Host Controller type
 */
struct sUSBHostDef
{
	tUSBIntOp	InterruptIN;
	tUSBIntOp	InterruptOUT;
	void	(*StopInterrupt)(void *Ptr, void *Handle);

	void	*(*ControlSETUP)(void *Ptr, int Dest, int DataTgl, void *Data, size_t Length);
	tUSBHostOp	ControlIN;
	tUSBHostOp	ControlOUT;
	
	tUSBHostOp	BulkIN;
	tUSBHostOp	BulkOUT;

	void	(*CheckPorts)(void *Ptr);
};

extern tUSBHub	*USB_RegisterHost(tUSBHostDef *HostDef, void *ControllerPtr, int nPorts);

#endif

