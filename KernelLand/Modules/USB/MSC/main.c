/*
 * Acess2 USB Stack Mass Storage Driver
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Driver Core
 */
#define DEBUG	1
#define VERSION	VER2(0,1)
#include <acess.h>
#include <modules.h>
#include "common.h"
#include "msc_proto.h"

// === PROTOTYPES ===
 int	MSC_Initialise(char **Arguments);
void	MSC_Cleanup(void);
void	MSC_DeviceConnected(tUSBInterface *Dev, void *Descriptors, size_t DescriptorsLen);
void	MSC_DataIn(tUSBInterface *Dev, int EndPt, int Length, void *Data);
// --- Internal Helpers
 int	MSC_int_CreateCBW(tMSC_CBW *Cbw, int bInput, size_t CmdLen, const void *CmdData, size_t DataLen);
void	MSC_SendData(tUSBInterface *Dev, size_t CmdLen, const void *CmdData, size_t DataLen, const void *Data);
void	MSC_RecvData(tUSBInterface *Dev, size_t CmdLen, const void *CmdData, size_t DataLen, void *Data);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, USB_MSC, MSC_Initialise, MSC_Cleanup, "USB_Core", NULL);
tUSBDriver	gMSC_Driver_BulkOnly = {
	.Name = "MSC_BulkOnly",
	.Match = {.Class = {0x080050, 0xFF00FF}},
	.Connected = MSC_DeviceConnected,
	.MaxEndpoints = 2,
	.Endpoints = {
		{0x82, MSC_DataIn},
		{0x02, NULL},
		}
	};
int giMSC_NextTag;

// === CODE ===
int MSC_Initialise(char **Arguments)
{
	USB_RegisterDriver( &gMSC_Driver_BulkOnly );
	return 0;
}

void MSC_Cleanup(void)
{
}

void MSC_DeviceConnected(tUSBInterface *Dev, void *Descriptors, size_t DescriptorsLen)
{
	tMSCInfo	*info;
	tLVM_VolType	*vt;
	
	info = malloc( sizeof(*info) );
	USB_SetDeviceDataPtr(Dev, info);
	
	// Get the class code (and hence what command set is in use)
	Uint8	sc = (USB_GetInterfaceClass(Dev) & 0x00FF00) >> 8;
	switch( sc )
	{
	// SCSI Transparent Command Set
	case 0x06:
		info->BlockCount = MSC_SCSI_GetSize(Dev, &info->BlockSize);
		vt = &gMSC_SCSI_VolType;
		break;
	// Unknown, prepare to chuck sad
	default:
		Log_Error("USB MSC", "Unknown sub-class 0x%02x", sc);
		USB_SetDeviceDataPtr(Dev, NULL);
		free(info);
		return ;
	}

	// Create device name from Vendor ID, Device ID and Serial Number
	Uint16	vendor, devid;
	char	*serial_number;
	USB_GetDeviceVendor(Dev, &vendor, &devid);
	serial_number = USB_GetSerialNumber(Dev);
	if( !serial_number ) {
		// No serial number - this breaks spec, but it's easy to handle
		Log_Warning("USB MSC", "Device does not have a serial number, using a random one");
		serial_number = malloc(4+8+1);
		sprintf(serial_number, "rand%08x", rand());
	}
	char	name[4 + 4 + 1 + 4 + 1 + strlen(serial_number) + 1];
	sprintf(name, "usb-%04x:%04x-%s", vendor, devid, serial_number);
	free(serial_number);

	// Pass the buck to LVM
	LOG("Device '%s' has 0x%llx blocks of 0x%x bytes", name, info->BlockCount, info->BlockSize);
	LVM_AddVolume(vt, name, Dev, info->BlockSize, info->BlockCount);
}

void MSC_DataIn(tUSBInterface *Dev, int EndPt, int Length, void *Data)
{
	// Will this ever be needed?
}

/**
 * \brief Create a CBW structure
 */
int MSC_int_CreateCBW(tMSC_CBW *Cbw, int bInput, size_t CmdLen, const void *CmdData, size_t DataLen)
{
	if( CmdLen == 0 ) {
		Log_Error("MSC", "Zero length commands are invalid");
		return -1;
	}
	
	if( CmdLen > 16 ) {
		Log_Error("MSC", "Command data >16 bytes (%i)", CmdLen);
		return -1;
	}
	
	Cbw->dCBWSignature = LittleEndian32(0x43425355);
	Cbw->dCBWTag    = giMSC_NextTag ++;
	Cbw->dCBWDataTransferLength = LittleEndian32(DataLen);
	Cbw->bmCBWFlags = (bInput ? 0x80 : 0x00);
	Cbw->bCBWLUN    = 0;	// TODO: Logical Unit Number (param from proto)
	Cbw->bCBWLength = CmdLen;
	memcpy(Cbw->CBWCB, CmdData, CmdLen);
	
	return 0;
}

void MSC_SendData(tUSBInterface *Dev, size_t CmdLen, const void *CmdData, size_t DataLen, const void *Data)
{
	tMSC_CBW	cbw;
	tMSC_CSW	csw;
	const int	endpoint_out = 2;
	const int	endpoint_in = 1;
	
	if( MSC_int_CreateCBW(&cbw, 0, CmdLen, CmdData, DataLen) )
		return ;
	
	// Send CBW
	USB_SendData(Dev, endpoint_out, sizeof(cbw), &cbw);
	
	// Send Data
	USB_SendData(Dev, endpoint_out, DataLen, Data);
	
	// Read CSW
	USB_RecvData(Dev, endpoint_in, sizeof(csw), &csw);
	// TODO: Validate CSW
}

void MSC_RecvData(tUSBInterface *Dev, size_t CmdLen, const void *CmdData, size_t DataLen, void *Data)
{
	tMSC_CBW	cbw;
	tMSC_CSW	csw;
	const int	endpoint_out = 2;
	const int	endpoint_in = 1;
		
	if( MSC_int_CreateCBW(&cbw, 1, CmdLen, CmdData, DataLen) )
		return ;
	
	// Send CBW
	USB_SendData(Dev, endpoint_out, sizeof(cbw), &cbw);
	
	// Read Data
	// TODO: use async version and wait for the transaction to complete
	USB_RecvData(Dev, endpoint_in, DataLen, Data);
	
	// Read CSW
	USB_RecvData(Dev, endpoint_in, sizeof(csw), &csw);
	// TODO: Validate CSW
}

