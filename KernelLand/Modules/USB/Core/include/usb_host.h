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

typedef void	*(*tUSBInitInt)(void *Ptr, int Endpt, int bOutbound, int Period, tUSBHostCb Cb, void *CbData, void *Buf, size_t Len);
typedef void	*(*tUSBInit)(void *Ptr, int Endpt, size_t MaxPacketSize);
typedef void	*(*tUSBDataOp)(void *Dest, tUSBHostCb Cb, void *CbData, void *Data, size_t Length);

typedef void	*(*tUSBControlOp)(void *Ptr, void *Endpt, tUSBHostCb Cb, void *CbData,
	int bOutbound,	// (1) SETUP, OUT, IN vs (0) SETUP, IN, OUT
	const void *SetupData, size_t SetupLength,
	const void *OutData, size_t OutLength,
	void *InData, size_t InLength
	);
typedef void	*(*tUSBBulkOp)(void *Ptr, void *Endpt, tUSBHostCb Cb, void *CbData,
	int bOutbound, void *Data, size_t Length
	);
typedef void	*(*tUSBIsochOp)(void *Ptr, void *Endpt, tUSBHostCb Cb, void *CbData,
	int bOutbound, void *Data, size_t Length, int When
	);

/**
 * \brief Defines a USB Host Controller type
 */
struct sUSBHostDef
{
	tUSBInitInt	InitInterrupt;
	tUSBInit	InitIsoch;
	tUSBInit	InitControl;
	tUSBInit	InitBulk;
	void	(*RemEndpoint)(void *Ptr, void *Handle);
	
	// NOTE: If \a Cb is ERRPTR, the handle returned must be free'd by the calling code
	//       otherwise the controller will free it once done
	tUSBIsochOp	SendIsoch;
	tUSBControlOp	SendControl;
	tUSBBulkOp	SendBulk;
	void	(*FreeOp)(void *Ptr, void *Handle);

	// Root hub stuff
	void	(*CheckPorts)(void *Ptr);
	void	(*SetPortFeature)(void *Ptr, int PortNum, int Feat);
	void	(*ClearPortFeature)(void *Ptr, int PortNum, int Feat);
	 int	(*GetPortStatus)(void *Ptr, int PortNum, int Flag);
};

extern tUSBHub	*USB_RegisterHost(tUSBHostDef *HostDef, void *ControllerPtr, int nPorts);

#endif

