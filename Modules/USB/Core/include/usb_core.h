/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb_core.h
 * - Core USB IO Header 
 */
#ifndef _USB_CORE_H_
#define _USB_CORE_H_

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

extern void	USB_SendData(tUSBInterface *Dev, int Endpoint, int Length, void *Data);

#endif

