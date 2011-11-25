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
	Uint8	Type;	// =
	Uint16	USBVersion;	// BCD, 0x210 = 2.10
	Uint8	DeviceClass;
	Uint8	DeviceSubClass;
	Uint8	DeviceProtocol;
	Uint8	MaxPacketSize;
	
	Uint16	VendorID;
	Uint16	ProductID;
	
	Uint8	ManufacturerStr;
	Uint8	ProductStr;
	Uint8	SerialNumberStr;
	
	Uint8	NumConfigurations;
};

#endif

