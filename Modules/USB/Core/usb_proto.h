/**
 */
#ifndef _USB_PROTO_H_
#define _USB_PROTO_H_

struct sDeviceRequest
{
	Uint8	ReqType;
	Uint8	Request;
	Uint16	Value;
	Uint16	Index;
	Uint16	Length;
};

/*
 */
struct sDescriptor_Device
{
	Uint8	Length;
	Uint8	Type;	// = 1
	Uint16	USBVersion;	// BCD, 0x210 = 2.10
	Uint8	DeviceClass;
	Uint8	DeviceSubClass;
	Uint8	DeviceProtocol;
	Uint8	MaxPacketSize;
	
	Uint16	VendorID;
	Uint16	ProductID;
	Uint16	DeviceID;	// BCD
	
	Uint8	ManufacturerStr;
	Uint8	ProductStr;
	Uint8	SerialNumberStr;
	
	Uint8	NumConfigurations;
} PACKED;

struct sDescriptor_Configuration
{
	Uint8	Length;
	Uint8	Type;	// = 2
	
	Uint16	TotalLength;
	Uint8	NumInterfaces;
	Uint8	ConfigurationValue;
	Uint8	ConfigurationStr;
	Uint8	AttributesBmp;
	Uint8	MaxPower;	// in units of 2 mA
} PACKED;

struct sDescriptor_String
{
	Uint8	Length;
	Uint8	Type;	// = 3
	
	Uint16	Data[62];	// 62 is arbitary
} PACKED;

struct sDescriptor_Interface
{
	Uint8	Length;
	Uint8	Type;	// = 4
	
	Uint8	InterfaceNum;
	Uint8	AlternateSetting;
	Uint8	NumEndpoints;	// Excludes endpoint 0
	
	Uint8	InterfaceClass;	// 
	Uint8	InterfaceSubClass;
	Uint8	InterfaceProtocol;
	
	Uint8	InterfaceStr;
} PACKED;

struct sDescriptor_Endpoint
{
	Uint8	Length;
	Uint8	Type;	// = 5
	Uint8	Address;	// 3:0 Endpoint Num, 7: Direction (IN/OUT)
	/**
	 * 1:0 - Transfer Type
	 * - 00 = Control
	 * - 01 = Isochronous
	 * - 10 = Bulk
	 * - 11 = Interrupt
	 * 3:2 - Synchronisation type (Isonchronous only)
	 * - 00 = No Synchronisation
	 * - 01 = Asynchronous
	 * - 10 = Adaptive
	 * - 11 = Synchronous
	 * 5:4 - Usage type (Isonchronous only)
	 * - 00 = Data endpoint
	 * - 01 = Feedback endpoint
	 */
	Uint8	Attributes;
	
	Uint16	MaxPacketSize;
	
	/**
	 * 
	 */
	Uint8	PollingInterval;
} PACKED;

#endif

