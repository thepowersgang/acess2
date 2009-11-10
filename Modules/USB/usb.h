/*
 * AcessOS Version 1
 * USB Stack
 */
#ifndef _USB_H_
#define _USB_H_

// === TYPES ===
typedef struct sUSBHost	tUSBHost;
typedef struct sUSBDevice	tUSBDevice;

// === STRUCTURES ===
/**
 * \brief Defines a USB Host Controller
 */
struct sUSBHost
{
	Uint16	IOBase;
	
	 int	(*SendPacket)(int ID, int Length, void *Data);
};

/**
 * \brief Defines a single device on the USB Bus
 */
struct sUSBDevice
{
	tUSBHost	*Host;
};

#endif
