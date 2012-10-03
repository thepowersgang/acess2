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

typedef void	(*tUSB_DataCallback)(tUSBInterface *Dev, int EndPt, int Length, void *Data);

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
			Uint32	ClassCode;
			Uint32	ClassMask;
		} Class;
		struct {
			Uint16	VendorID;
			Uint16	DeviceID;
		} VendorDev;
	} Match;

	void	(*Connected)(tUSBInterface *Dev, void *Descriptors, size_t Size);
	void	(*Disconnected)(tUSBInterface *Dev);

	 int	MaxEndpoints;	
	struct {
		// USB Attrbute byte
		// NOTE: Top bit indicates the direction (1=Input)
		Uint8	Attributes;
		// Data availiable Callback
		tUSB_DataCallback	DataAvail;
	} Endpoints[];
};

extern void	USB_RegisterDriver(tUSBDriver *Driver);

// --- Driver Pointer ---
extern void	*USB_GetDeviceDataPtr(tUSBInterface *Dev);
extern void	USB_SetDeviceDataPtr(tUSBInterface *Dev, void *Ptr);

// --- Device/Interface information ---
extern Uint32	USB_GetInterfaceClass(tUSBInterface *Dev);
extern void	USB_GetDeviceVendor(tUSBInterface *Dev, Uint16 *VendorID, Uint16 *DeviceID);
extern char	*USB_GetSerialNumber(tUSBInterface *Dev);

// --- Device IO ---
extern void	USB_StartPollingEndpoint(tUSBInterface *Dev, int Endpoint);
extern void	USB_ReadDescriptor(tUSBInterface *Dev, int Type, int Index, int Length, void *Data);
extern void	USB_Request(tUSBInterface *Dev, int Endpoint, int Type, int Req, int Value, int Index, int Len, void *Data);
// TODO: Async
extern void	USB_SendData(tUSBInterface *Dev, int Endpoint, size_t Length, const void *Data);
extern void	USB_RecvData(tUSBInterface *Dev, int Endpoint, size_t Length, void *Data);
extern void	USB_RecvDataA(tUSBInterface *Dev, int Endpoint, size_t Length, void *DataBuf);

#endif

