/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * hub.c
 * - Basic hub driver
 */
#define DEBUG	0
#include <usb_hub.h>
#include <timers.h>

#define MAX_PORTS	32	// Not actually a max, but used for DeviceRemovable


#define GET_STATUS	0
#define CLEAR_FEATURE	1
// resvd
#define SET_FEATURE	3

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

struct sHubInfo
{
	tUSBHub	*HubPtr;
	 int	PowerOnDelay;	// in ms
	 int	nPorts;
	Uint8	DeviceRemovable[];
};

// === PROTOTYPES ===
void	Hub_Connected(tUSBInterface *Dev, void *Descriptors, size_t Length);
void	Hub_Disconnected(tUSBInterface *Dev);
void	Hub_PortStatusChange(tUSBInterface *Dev, int Endpoint, int Length, void *Data);
void	Hub_int_HandleChange(tUSBInterface *Dev, int Port);
void	Hub_SetPortFeature(tUSBInterface *Dev, int Port, int Feat);
void	Hub_ClearPortFeature(tUSBInterface *HubDev, int Port, int Feat);
int	Hub_GetPortStatus(tUSBInterface *HubDev, int Port, int Flag);

// === GLOBALS ===
tUSBDriver	gUSBHub_Driver = {
	.Name = "Hub",
	.Match = {.Class = {0x090000, 0xFF0000}},
	.Connected = Hub_Connected,
	.Disconnected = Hub_Disconnected,
	.MaxEndpoints = 1,
	.Endpoints = {
		{0x83, Hub_PortStatusChange}
	}
};

// === CODE ===
#if 0
int Hub_DriverInitialise(char **Arguments)
{
	USB_RegisterDriver( &gUSBHub_Driver );
	return 0;
}
#endif

void Hub_Connected(tUSBInterface *Dev, void *Descriptors, size_t Length)
{
	struct sHubDescriptor	hub_desc;
	struct sHubInfo	*info;	

	// Read hub descriptor (Class descriptor 0x9, destined for device)
	USB_ReadDescriptor(Dev, 0x0129, 0, sizeof(hub_desc), &hub_desc);

	LOG("%i Ports", hub_desc.NbrPorts);
	LOG("Takes %i ms for power to stabilise", hub_desc.PwrOn2PwrGood*2);

	// Allocate infomation structure
	info = malloc(sizeof(*info) + (hub_desc.NbrPorts+7)/8);
	if(!info) {
		Log_Error("USBHub", "malloc() failed");
		return ;
	}
	USB_SetDeviceDataPtr(Dev, info);

	// Fill data
	info->nPorts = hub_desc.NbrPorts;
	info->PowerOnDelay = hub_desc.PwrOn2PwrGood * 2;
	memcpy(info->DeviceRemovable, hub_desc.DeviceRemovable, (hub_desc.NbrPorts+7)/8);
	// Register
	info->HubPtr = USB_RegisterHub(Dev, info->nPorts);

	// Register poll on endpoint
	USB_StartPollingEndpoint(Dev, 1);
}

void Hub_Disconnected(tUSBInterface *Dev)
{
	struct sHubInfo	*info = USB_GetDeviceDataPtr(Dev);
	USB_RemoveHub(info->HubPtr);
	free(info);
}

void Hub_PortStatusChange(tUSBInterface *Dev, int Endpoint, int Length, void *Data)
{
	Uint8	*status = Data;
	struct sHubInfo	*info = USB_GetDeviceDataPtr(Dev);
	 int	i;
	for( i = 0; i < info->nPorts; i += 8, status ++ )
	{
		if( i/8 >= Length )	break;
		LOG("status[%i] = %x", i/8, *status);
		if( *status == 0 )	continue;	

		for( int j = 0; j < 8; j ++ )
			if( *status & (1 << j) )
				Hub_int_HandleChange(Dev, i+j);
	}
}

void Hub_int_HandleChange(tUSBInterface *Dev, int Port)
{
	struct sHubInfo	*info = USB_GetDeviceDataPtr(Dev);
	Uint16	status[2];	// Status, Change
	
	// Get port status
	USB_Request(Dev, 0, 0xA3, GET_STATUS, 0, Port, 4, status);

	LOG("Port %i: status = {0b%b, 0b%b}", Port, status[0], status[1]);
	
	// Handle connections / disconnections
	if( status[1] & 0x0001 )
	{
		if( status[0] & 0x0001 ) {
			// Connected
			// - Power on port
			USB_Request(Dev, 0, 0x23, SET_FEATURE, PORT_POWER, Port, 0, NULL);
			Time_Delay(info->PowerOnDelay);
			// - Start reset process
			USB_PortCtl_BeginReset(info->HubPtr, Port);
		}
		else {
			// Disconnected
			USB_DeviceDisconnected(info->HubPtr, Port);
		}
		
		USB_Request(Dev, 0, 0x23, CLEAR_FEATURE, C_PORT_CONNECTION, Port, 0, NULL);
	}
	
	// Reset change
	if( status[1] & 0x0010 )
	{
		if( status[0] & 0x0010 ) {
			// Reset complete
		}
		else {
			// Useful?
		}
		// ACK
		USB_Request(Dev, 0, 0x23, CLEAR_FEATURE, C_PORT_RESET, Port, 0, NULL);
	}
}

void Hub_SetPortFeature(tUSBInterface *Dev, int Port, int Feat)
{
	USB_Request(Dev, 0, 0x23, SET_FEATURE, Feat, Port, 0, NULL);
}

void Hub_ClearPortFeature(tUSBInterface *Dev, int Port, int Feat)
{
	USB_Request(Dev, 0, 0x23, CLEAR_FEATURE, Feat, Port, 0, NULL);
}

int Hub_GetPortStatus(tUSBInterface *Dev, int Port, int Flag)
{
	Uint16	status[2];	// Status, Change
	USB_Request(Dev, 0, 0xA3, GET_STATUS, 0, Port, 4, status);
	return !!(status[0] & (1 << Flag));
}
