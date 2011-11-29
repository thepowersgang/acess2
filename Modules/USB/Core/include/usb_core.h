/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb_core.h
 * - Core USB IO Header 
 */
#ifndef _USB_CORE_H_
#define _USB_CORE_H_

#include <acess.h>

typedef struct sUSBInterface	tUSBInterface;
typedef struct sUSBDriver	tUSBDriver;

/**
 */
struct sUSBDriver
{
	tUSBDriver	*Next;
	
	const char	*Name;

	 int	MatchType;	// 0: Interface, 1: Device, 2: Vendor
	union {	
		struct {
			// 23:16 - Interface Class
			// 15:8  - Interface Sub Class
			// 7:0   - Interface Protocol
			Uint32	ClassMask;
			Uint32	ClassCode;
		} Class;
		struct {
			Uint16	VendorID;
			Uint16	DeviceID;
		} VendorDev;
	} Match;

	void	(*Connected)(tUSBInterface *Dev);
	void	(*Disconnected)(tUSBInterface *Dev);

	 int	MaxEndpoints;	
	struct {
		// USB Attrbute byte
		// NOTE: Top bit indicates the direction (1=Input)
		Uint8	Attributes;
		// Data availiable Callback
		void	(*DataAvail)(tUSBInterface *Dev, int Length, void *Data);
	} Endpoints[];
};

extern void	*USB_GetDeviceDataPtr(tUSBInterface *Dev);
extern void	USB_SetDeviceDataPtr(tUSBInterface *Dev, void *Ptr);

extern void	USB_StartPollingEndpoint(tUSBInterface *Dev, int Endpoint);
extern void	USB_ReadDescriptor(tUSBInterface *Dev, int Type, int Index, int Length, void *Data);
extern void	USB_Request(tUSBInterface *Dev, int Endpoint, int Type, int Req, int Value, int Index, int Len, void *Data);
// TODO: Async
extern void	USB_SendData(tUSBInterface *Dev, int Endpoint, int Length, void *Data);
extern void	USB_RecvData(tUSBInterface *Dev, int Endpoint, int Length, void *Data);

#endif

