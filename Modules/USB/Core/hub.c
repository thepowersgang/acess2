/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * hub.c
 * - Basic hub driver
 */
#include <usb_hub.h>

#define MAX_PORTS	32	// Not actually a max, but used for DeviceRemovable

struct sHubDescriptor
{
	Uint8	DescLength;
	Uint8	DescType;	// = 0x29
	Uint8	NbrPorts;
	Uint16	HubCharacteristics;
	Uint8	PwrOn2PwrGood;	// 2 ms intervals
	Uint8	HubContrCurrent;	// Max internal current (mA)
	Uint8	DeviceRemovable[MAX_PORTS];
};

// === PROTOTYPES ===
void	Hub_Connected(tUSBInterface *Dev);
void	Hub_Disconnected(tUSBInterface *Dev);
void	Hub_PortStatusChange(tUSBInterface *Dev, int Length, void *Data);

// === GLOBALS ===
tUSBDriver	gUSBHub_Driver = {
	.Name = "Hub",
	.Match = {.Class = {0x090000, 0xFF0000}},
	.Connected = Hub_Connected,
	.Disconnected = Hub_Disconnected,
	.MaxEndpoints = 1,
	.Endpoints = {
		{0x83, Hub_PortStatusChange}
	};
};

// === CODE ===
void Hub_Connected(tUSBInterface *Dev)
{
	struct sHubDescriptor	*hub_desc;
	
	hub_desc = malloc(sizeof(*hub_desc));
	if(!hub_desc) {
		Log_Error("USBHub", "malloc() failed");
		return ;
	}
	USB_SetDeviceDataPtr(Dev, hub_desc);

	USB_ReadDescriptor(Dev, 0, 0x29, 0, sizeof(hub_desc), hub_desc);

	// Register poll on endpoint
	USB_PollEndpoint(Dev, 1);
	
	USB_RegisterHub(Dev, hub_desc->NbrPorts);
}

void Hub_Disconnected(tUSBInterface *Dev)
{
}

void Hub_PortStatusChange(tUSBInterface *Dev, int Length, void *Data)
{
	Uint8	*status = Data;
	struct sHubDescriptor	*info = USB_GetDeviceDataPtr(Dev);
	 int	i;
	for( i = 0; i < info->NbrPorts; i += 8, status ++ )
	{
		if( i/8 >= Length )	break;
		if( *status == 0 )	continue;
	
		for( int j = 0; j < 8; j ++ )
			if( *status & (1 << j) )
				Hub_int_HandleChange(Dev, i+j);
	}
}

void Hub_int_HandleChange(tUSBInterface *Dev, int Port)
{
	Uint16	status[2];	// Status, Change
	// Get change status
	USB_Request(Dev, 0, 0xA3, 0, 0, Port, 4, status);
}

