/*
 * Acess 2 USB Stack
 * USB Packet Control
 */
#define DEBUG	1
#include <acess.h>
#include <vfs.h>
#include <drv_pci.h>
#include "usb.h"
#include "usb_proto.h"

// === IMPORTS ===
extern tUSBHost	*gUSB_Hosts;

// === STRUCTURES ===

// === PROTOTYPES ===
tUSBHub	*USB_RegisterHost(tUSBHostDef *HostDef, void *ControllerPtr, int nPorts);
void	USB_DeviceConnected(tUSBHub *Hub, int Port);
void	USB_DeviceDisconnected(tUSBHub *Hub, int Port);
void	*USB_GetDeviceDataPtr(tUSBDevice *Dev);
void	USB_SetDeviceDataPtr(tUSBDevice *Dev, void *Ptr);
 int	USB_int_AllocateAddress(tUSBHost *Host);
 int	USB_int_SendSetupSetAddress(tUSBHost *Host, int Address);
 int	USB_int_ReadDescriptor(tUSBDevice *Dev, int Endpoint, int Type, int Index, int Length, void *Dest);
char	*USB_int_GetDeviceString(tUSBDevice *Dev, int Endpoint, int Index);

 int	_UTF16to8(Uint16 *Input, int InputLen, char *Dest);

// === CODE ===
tUSBHub *USB_RegisterHost(tUSBHostDef *HostDef, void *ControllerPtr, int nPorts)
{
	tUSBHost	*host;
	
	host = malloc(sizeof(tUSBHost) + nPorts*sizeof(void*));
	if(!host) {
		// Oh, bugger.
		return NULL;
	}
	host->HostDef = HostDef;
	host->Ptr = ControllerPtr;
	memset(host->AddressBitmap, 0, sizeof(host->AddressBitmap));

	host->RootHubDev.Next = NULL;
	host->RootHubDev.ParentHub = NULL;
	host->RootHubDev.Host = host;
	host->RootHubDev.Address = 0;
	host->RootHubDev.Driver = NULL;
	host->RootHubDev.Data = NULL;

	host->RootHub.Device = &host->RootHubDev;
	host->RootHub.CheckPorts = NULL;
	host->RootHub.nPorts = nPorts;
	memset(host->RootHub.Devices, 0, sizeof(void*)*nPorts);

	// TODO: Lock
	host->Next = gUSB_Hosts;
	gUSB_Hosts = host;

	return &host->RootHub;
}

void USB_DeviceConnected(tUSBHub *Hub, int Port)
{
	tUSBDevice	*dev;
	if( Port >= Hub->nPorts )	return ;
	if( Hub->Devices[Port] )	return ;

	ENTER("pHub iPort", Hub, Port);

	// 0. Perform port init? (done in hub?)	
	
	// Create structure
	dev = malloc(sizeof(tUSBDevice));
	dev->Next = NULL;
	dev->ParentHub = Hub;
	dev->Host = Hub->Device->Host;
	dev->Address = 0;
	dev->Driver = 0;
	dev->Data = 0;

	// 1. Assign an address
	dev->Address = USB_int_AllocateAddress(dev->Host);
	if(dev->Address == 0) {
		Log_Error("USB", "No addresses avaliable on host %p", dev->Host);
		free(dev);
		LEAVE('-');
		return ;
	}
	USB_int_SendSetupSetAddress(dev->Host, dev->Address);
	LOG("Assigned address %i", dev->Address);
	
	// 2. Get device information
	{
		struct sDescriptor_Device	desc;
		LOG("Getting device descriptor");
		// Endpoint 0, Desc Type 1, Index 0
		USB_int_ReadDescriptor(dev, 0, 1, 0, sizeof(desc), &desc);
		
		LOG("Device Descriptor = {");
		LOG(" .Length = %i", desc.Length);
		LOG(" .Type = %i", desc.Type);
		LOG(" .USBVersion = 0x%04x", desc.USBVersion);
		LOG(" .DeviceClass = 0x%02x", desc.DeviceClass);
		LOG(" .DeviceSubClass = 0x%02x", desc.DeviceSubClass);
		LOG(" .DeviceProtocol = 0x%02x", desc.DeviceProtocol);
		LOG(" .MaxPacketSize = 0x%02x", desc.MaxPacketSize);
		LOG(" .VendorID = 0x%04x", desc.VendorID);
		LOG(" .ProductID = 0x%04x", desc.ProductID);
		LOG(" .DeviceID = 0x%04x", desc.DeviceID);
		LOG(" .ManufacturerStr = Str %i", desc.ManufacturerStr);
		LOG(" .ProductStr = Str %i", desc.ProductStr);
		LOG(" .SerialNumberStr = Str %i", desc.SerialNumberStr);
		LOG(" .NumConfigurations = %i", desc.SerialNumberStr);
		LOG("}");
		
		if( desc.ManufacturerStr )
		{
			char	*tmp = USB_int_GetDeviceString(dev, 0, desc.ManufacturerStr);
			LOG("ManufacturerStr = '%s'", tmp);
			free(tmp);
		}
		if( desc.ProductStr )
		{
			char	*tmp = USB_int_GetDeviceString(dev, 0, desc.ProductStr);
			LOG("ProductStr = '%s'", tmp);
			free(tmp);
		}
		if( desc.SerialNumberStr )
		{
			char	*tmp = USB_int_GetDeviceString(dev, 0, desc.SerialNumberStr);
			LOG("SerialNumbertStr = '%s'", tmp);
			free(tmp);
		}
	}
	
	// 3. Get configurations
	for( int i = 0; i < 1; i ++ )
	{
		struct sDescriptor_Configuration	desc;
//		void	*full_buf;
		
		USB_int_ReadDescriptor(dev, 0, 2, 0, sizeof(desc), &desc);
		LOG("Configuration Descriptor %i = {", i);
		LOG(" .Length = %i", desc.Length);
		LOG(" .Type = %i", desc.Type);
		LOG(" .TotalLength = 0x%x", LittleEndian16(desc.TotalLength));
		LOG(" .NumInterfaces = %i", desc.NumInterfaces);
		LOG(" .ConfigurationValue = %i", desc.ConfigurationValue);
		LOG(" .ConfigurationStr = %i", desc.ConfigurationStr);
		LOG(" .AttributesBmp = 0b%b", desc.AttributesBmp);
		LOG(" .MaxPower = %i (*2mA)", desc.MaxPower);
		LOG("}");
		if( desc.ConfigurationStr ) {
			char	*tmp = USB_int_GetDeviceString(dev, 0, desc.ConfigurationStr);
			LOG("ConfigurationStr = '%s'", tmp);
			free(tmp);
		}
		
		// TODO: Interfaces
	}

	// Done.
	LEAVE('-');
}

void USB_DeviceDisconnected(tUSBHub *Hub, int Port)
{
	
}

void *USB_GetDeviceDataPtr(tUSBDevice *Dev) { return Dev->Data; }
void USB_SetDeviceDataPtr(tUSBDevice *Dev, void *Ptr) { Dev->Data = Ptr; }

int USB_int_AllocateAddress(tUSBHost *Host)
{
	 int	i;
	for( i = 1; i < 128; i ++ )
	{
		if(Host->AddressBitmap[i/8] & (1 << (i%8)))
			continue ;
		Host->AddressBitmap[i/8] |= 1 << (i%8);
		return i;
	}
	return 0;
}

void USB_int_DeallocateAddress(tUSBHost *Host, int Address)
{
	Host->AddressBitmap[Address/8] &= ~(1 << (Address%8));
}

int USB_int_SendSetupSetAddress(tUSBHost *Host, int Address)
{
	void	*hdl;
	struct sDeviceRequest	req;
	req.ReqType = 0;	// bmRequestType
	req.Request = 5;	// SET_ADDRESS
	// TODO: Endian
	req.Value = LittleEndian16( Address & 0x7F );	// wValue
	req.Index = LittleEndian16( 0 );	// wIndex
	req.Length = LittleEndian16( 0 );	// wLength
	
	// Addr 0:0, Data Toggle = 0, no interrupt
	hdl = Host->HostDef->SendSETUP(Host->Ptr, 0, 0, 0, FALSE, &req, sizeof(req));
	if(!hdl)
		return 1;

	// TODO: Data toggle?
	hdl = Host->HostDef->SendIN(Host->Ptr, 0, 0, 0, FALSE, NULL, 0);
	
	while( Host->HostDef->IsOpComplete(Host->Ptr, hdl) == 0 )
		Time_Delay(1);
	
	return 0;
}

int USB_int_ReadDescriptor(tUSBDevice *Dev, int Endpoint, int Type, int Index, int Length, void *Dest)
{
	const int	ciMaxPacketSize = 0x400;
	struct sDeviceRequest	req;
	 int	bToggle = 0;
	void	*final;

	req.ReqType = 0x80;
	req.Request = 6;	// GET_DESCRIPTOR
	req.Value = LittleEndian16( ((Type & 0xFF) << 8) | (Index & 0xFF) );
	req.Index = LittleEndian16( 0 );	// TODO: Language ID
	req.Length = LittleEndian16( Length );
	
	Dev->Host->HostDef->SendSETUP(
		Dev->Host->Ptr, Dev->Address, Endpoint,
		0, NULL,
		&req, sizeof(req)
		);
	
	bToggle = 1;
	while( Length > ciMaxPacketSize )
	{
		Dev->Host->HostDef->SendIN(
			Dev->Host->Ptr, Dev->Address, Endpoint,
			bToggle, NULL,
			Dest, ciMaxPacketSize
			);
		bToggle = !bToggle;
		Length -= ciMaxPacketSize;
	}

	final = Dev->Host->HostDef->SendIN(
		Dev->Host->Ptr, Dev->Address, Endpoint,
		bToggle, INVLPTR,
		Dest, Length
		);

	while( Dev->Host->HostDef->IsOpComplete(Dev->Host->Ptr, final) == 0 )
		Time_Delay(1);

	return 0;
}

char *USB_int_GetDeviceString(tUSBDevice *Dev, int Endpoint, int Index)
{
	struct sDescriptor_String	str;
	 int	src_len, new_len;
	char	*ret;
	
	USB_int_ReadDescriptor(Dev, Endpoint, 3, Index, sizeof(str), &str);
	if(str.Length > sizeof(str)) {
		Log_Notice("USB", "String is %i bytes, which is over prealloc size (%i)",
			str.Length, sizeof(str)
			);
		// HACK: 
		str.Length = sizeof(str);
	}
	src_len = (str.Length - 2) / sizeof(str.Data[0]);

	new_len = _UTF16to8(str.Data, src_len, NULL);	
	ret = malloc( new_len + 1 );
	_UTF16to8(str.Data, src_len, ret);
	ret[new_len] = 0;
	return ret;
}

int _UTF16to8(Uint16 *Input, int InputLen, char *Dest)
{
	 int	str_len, cp_len;
	Uint32	saved_bits = 0;
	str_len = 0;
	for( int i = 0; i < InputLen; i ++)
	{
		Uint32	cp;
		Uint16	val = Input[i];
		if( val >= 0xD800 && val <= 0xDBFF )
		{
			// Multibyte - Leading
			if(i + 1 > InputLen) {
				cp = '?';
			}
			else {
				saved_bits = (val - 0xD800) << 10;
				saved_bits += 0x10000;
				continue ;
			}
		}
		else if( val >= 0xDC00 && val <= 0xDFFF )
		{
			if( !saved_bits ) {
				cp = '?';
			}
			else {
				saved_bits |= (val - 0xDC00);
				cp = saved_bits;
			}
		}
		else
			cp = val;

		cp_len = WriteUTF8((Uint8*)Dest, cp);
		if(Dest)
			Dest += cp_len;
		str_len += cp_len;

		saved_bits = 0;
	}
	
	return str_len;
}

