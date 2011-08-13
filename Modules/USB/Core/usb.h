/*
 * AcessOS Version 1
 * USB Stack
 */
#ifndef _USB_H_
#define _USB_H_

// === TYPES ===
typedef struct sUSBHost	tUSBHost;
typedef struct sUSBDevice	tUSBDevice;

// === CONSTANTS ===
enum eUSB_TransferType
{
	TRANSTYPE_ISYNCH,	// Constant, Low latency, low bandwidth, no transmission retries
	TRANSTYPE_INTERRUPT,	// -- NEVER SENT -- Spontanious, Low latency, low bandwith
	TRANSTYPE_CONTROL,	// Device control
	TRANSTYPE_BULK  	// High latency, high bandwidth
};

enum eUSB_PIDs
{
	/**
	 * \name Token
	 * \{
	 */
	PID_OUT		= 0xE1,
	PID_IN		= 0x69,
	PID_SOF		= 0xA5,
	PID_SETUP	= 0x2D,
	/**
	 * \}
	 */
	
	/**
	 * \name Data
	 * \{
	 */
	PID_DATA0	= 0xC3,
	PID_DATA1	= 0x4B,
	PID_DATA2	= 0x87,	// USB2 only
	PID_MDATA	= 0x0F,	// USB2 only
	/**
	 * \}
	 */
	
	/**
	 * \name Handshake
	 * \{
	 */
	PID_ACK		= 0xD2,
	PID_NAK		= 0x5A,
	PID_STALL	= 0x1E,
	PID_NYET	= 0x96,
	/**
	 * \}
	 */
	
	/**
	 * \name Special
	 * \{
	 */
	PID_PRE		= 0x3C, PID_ERR		= 0x3C,
	PID_SPLIT	= 0x78,
	PID_PING	= 0xB4,
	PID_RESVD	= 0xF0,
	/**
	 * \}
	 */
};

// === FUNCTIONS ===
/**
 * \note 00101 - X^5+X^2+1
 */
extern Uint8	USB_TokenCRC(void *Data, int len);
/**
 * \note X^16 + X15 + X^2 + 1
 */
extern Uint16	USB_DataCRC(void *Data, int len);

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
	 int	MaxControl;
	 int	MaxBulk;
	 int	MaxISync;
};

#endif
