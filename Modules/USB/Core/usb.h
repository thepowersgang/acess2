/*
 * AcessOS Version 1
 * USB Stack
 */
#ifndef _USB_H_
#define _USB_H_

// === TYPES ===
typedef struct sUSBHost	tUSBHost;
typedef struct sUSBHub	tUSBHub;
typedef struct sUSBDevice	tUSBDevice;

// === STRUCTURES ===
/**
 * \brief Defines a USB Host Controller type
 */
struct sUSBHost
{
	tUSBHost	*Next;

	void	(*CheckPorts)(void *Ptr);

	void	*(*SendIN)(void *Ptr, int Fcn, int Endpt, int DataTgl, int bIOC, void *Data, size_t Length);
	void	*(*SendOUT)(void *Ptr, int Fcn, int Endpt, int DataTgl, int bIOC, void *Data, size_t Length);
	void	*(*SendSETUP)(void *Ptr, int Fcn, int Endpt, int DataTgl, int bIOC, void *Data, size_t Length);
};

/**
 * \brief USB Hub data
 */
struct sUSBHub
{
	/**
	 * \brief Host controller used
	 */
	tUSBHost	*HostDef;
	void	*Controller;

	 int	nPorts;
	tUSBDevice	*Devices[];
};

/**
 * \brief Defines a single device on the USB Bus
 */
struct sUSBDevice
{
	tUSBDevice	*Next;
	tUSBDevice	*Hub;

	 int	Address;
	
	 int	Type;
	
	union {
		tUSBHub	Hub;
		char	Impl[0];
	}	Data;
};

extern void	USB_RegisterHost(tUSBHost *HostDef, void *ControllerPtr);
extern void	USB_NewDevice(tUSBHub *Hub);

#endif
