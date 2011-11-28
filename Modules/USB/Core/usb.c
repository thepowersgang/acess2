/*
 * Acess 2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb.c
 * - USB Packet Control
 */
#define DEBUG	1
#include <acess.h>
#include <vfs.h>
#include <drv_pci.h>
#include "usb.h"
#include "usb_proto.h"
#include "usb_lowlevel.h"

// === IMPORTS ===
extern tUSBHost	*gUSB_Hosts;

// === STRUCTURES ===

// === PROTOTYPES ===
tUSBHub	*USB_RegisterHost(tUSBHostDef *HostDef, void *ControllerPtr, int nPorts);
void	USB_DeviceConnected(tUSBHub *Hub, int Port);
void	USB_DeviceDisconnected(tUSBHub *Hub, int Port);
void	*USB_GetDeviceDataPtr(tUSBInterface *Dev);
void	USB_SetDeviceDataPtr(tUSBInterface *Dev, void *Ptr);
 int	USB_int_AllocateAddress(tUSBHost *Host);

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

	host->RootHubDev.ParentHub = NULL;
	host->RootHubDev.Host = host;
	host->RootHubDev.Address = 0;

	host->RootHubIf.Next = NULL;
	host->RootHubIf.Dev = &host->RootHubDev;
	host->RootHubIf.Driver = NULL;
	host->RootHubIf.Data = NULL;
	host->RootHubIf.nEndpoints = 0;

	host->RootHub.Interface = &host->RootHubIf;
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
	dev->ParentHub = Hub;
	dev->Host = Hub->Interface->Dev->Host;
	dev->Address = 0;

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
		void	*full_buf;
		char	*cur_ptr;
		
		USB_int_ReadDescriptor(dev, 0, 2, i, sizeof(desc), &desc);
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

		cur_ptr = full_buf = malloc( LittleEndian16(desc.TotalLength) );
		USB_int_ReadDescriptor(dev, 0, 2, i, desc.TotalLength, full_buf);

		cur_ptr += desc.Length;

		// TODO: Interfaces
		for( int j = 0; j < desc.NumInterfaces; j ++ )
		{
			struct sDescriptor_Interface *iface;
			iface = (void*)cur_ptr;
			// TODO: Sanity check with remaining space
			cur_ptr += sizeof(*iface);
			
			LOG("Interface %i/%i = {", i, j);
			LOG(" .InterfaceNum = %i", iface->InterfaceNum);
			LOG(" .NumEndpoints = %i", iface->NumEndpoints);
			LOG(" .InterfaceClass = 0x%x", iface->InterfaceClass);
			LOG(" .InterfaceSubClass = 0x%x", iface->InterfaceSubClass);
			LOG(" .InterfaceProcol = 0x%x", iface->InterfaceProtocol);

			if( iface->InterfaceStr ) {
				char	*tmp = USB_int_GetDeviceString(dev, 0, iface->InterfaceStr);
				LOG(" .InterfaceStr = %i '%s'", iface->InterfaceStr, tmp);
				free(tmp);
			}
			LOG("}");

			for( int k = 0; k < iface->NumEndpoints; k ++ )
			{
				struct sDescriptor_Endpoint *endpt;
				endpt = (void*)cur_ptr;
				// TODO: Sanity check with remaining space
				cur_ptr += sizeof(*endpt);
				
				LOG("Endpoint %i/%i/%i = {", i, j, k);
				LOG(" .Address = 0x%2x", endpt->Address);
				LOG(" .Attributes = 0b%8b", endpt->Attributes);
				LOG(" .MaxPacketSize = %i", LittleEndian16(endpt->MaxPacketSize));
				LOG(" .PollingInterval = %i", endpt->PollingInterval);
				LOG("}");
			}
		}
		
		free(full_buf);
	}

	// Done.
	LEAVE('-');
}

void USB_DeviceDisconnected(tUSBHub *Hub, int Port)
{
	
}

void *USB_GetDeviceDataPtr(tUSBInterface *Dev) { return Dev->Data; }
void USB_SetDeviceDataPtr(tUSBInterface *Dev, void *Ptr) { Dev->Data = Ptr; }

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

