/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb_core.h
 * - Core USB IO Header 
 */
#ifndef _USB_CORE_H_
#define _USB_CORE_H_

typedef struct sUSBDevice	tUSBDevice;
typedef struct sUSBDriver	tUSBDriver;

/**
 */
struct sUSBDriver
{
	tUSBDriver	*Next;
	
	const char	*Name;
	
	// 23:16 - Interface Class
	// 15:8  - Interface Sub Class
	// 7:0   - Interface Protocol
	Uint32	ClassMask;
	Uint32	ClassCode;

	void	(*Connected)(tUSBDevice *Dev);
	void	(*Disconnected)(tUSBDevice *Dev);

	 int	MaxEndpoints;	
	struct {
		// 0: Bulk, 1: Control, 2: Interrupt
		 int	Type;
		// Data availiable Callback
		void	(*Interrupt)(tUSBDevice *Dev, int Length, void *Data);
	} Endpoints[];
};

extern void	*USB_GetDeviceDataPtr(tUSBDevice *Dev);
extern void	USB_SetDeviceDataPtr(tUSBDevice *Dev, void *Ptr);

extern void	USB_SendData(tUSBDevice *Dev, int Endpoint, int Length, void *Data);

#endif

