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
	 int	(*SendIN)(void *Ptr, int Fcn, int Endpt, int DataTgl, void *Data, size_t Length);
	 int	(*SendOUT)(void *Ptr, int Fcn, int Endpt, int DataTgl, void *Data, size_t Length);
	 int	(*SendSETUP)(void *Ptr, int Fcn, int Endpt, int DataTgl, void *Data, size_t Length);
};

/**
 * \brief Defines a single device on the USB Bus
 */
struct sUSBDevice
{
	tUSBHost	*Host;

	 int	Address;

	 int	MaxControl;
	 int	MaxBulk;
	 int	MaxISync;
};

#endif
