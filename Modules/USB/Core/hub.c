/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * hub.c
 * - Basic hub driver
 */
#include <usb_hub.h>

struct sHubInfo
{
	 int	nPorts;
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
	// Register poll on endpoint
	USB_PollEndpoint(Dev, 0);
	
	USB_RegisterHub(Dev, nPorts);
}

void Hub_Disconnected(tUSBInterface *Dev)
{
}

void Hub_PortStatusChange(tUSBInterface *Dev, int Length, void *Data)
{
	 int	i;
	Uint8	*status = Data;
	for( i = 0; i < info->nPorts; i += 8, status ++ )
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

