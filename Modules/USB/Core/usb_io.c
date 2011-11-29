/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb_io.c
 * - High-level IO
 */
#define DEBUG	1

#include <usb_core.h>
#include "usb.h"
#include "usb_lowlevel.h"
#include <semaphore.h>

typedef struct sAsyncOp	tAsyncOp;

struct sAsyncOp
{
	tAsyncOp	*Next;
	tUSBEndpoint	*Endpt;
	 int	Length;
	void	*Data;
};

// === PROTOTYPES ===
void	USB_ReadDescriptor(tUSBInterface *Iface, int Type, int Index, int Length, void *Data);
void	USB_Request(tUSBInterface *Iface, int Endpoint, int Type, int Req, int Value, int Index, int Len, void *Data);

// === GLOBALS ===
tSemaphore	glUSB_AsyncQueue;

// === CODE ===
void USB_ReadDescriptor(tUSBInterface *Iface, int Type, int Index, int Length, void *Data)
{
	USB_int_ReadDescriptor(Iface->Dev, 0, Type, Index, Length, Data);
}

void USB_Request(tUSBInterface *Iface, int Endpoint, int Type, int Req, int Value, int Index, int Len, void *Data)
{
	 int	endpt;

	// Sanity check
	if(Endpoint < 0 || Endpoint >= Iface->nEndpoints)
		return ;	

	// Get endpoint number
	if(Endpoint)
		endpt = Iface->Endpoints[Endpoint-1].EndpointNum;
	else
		endpt = 0;
	
	USB_int_Request(Iface->Dev->Host, Iface->Dev->Address, endpt, Type, Req, Value, Index, Len, Data);
}


void USB_SendData(tUSBInterface *Dev, int Endpoint, int Length, void *Data)
{
	Log_Warning("USB", "TODO: Implement USB_SendData");
}

void USB_RecvData(tUSBInterface *Dev, int Endpoint, int Length, void *Data)
{
	Log_Warning("USB", "TODO: Implement USB_RecvData");
}

void USB_RecvDataA(tUSBInterface *Dev, int Endpoint, int Length, void *DataBuf, tUSB_DataCallback Callback)
{
	Log_Warning("USB", "TODO: Implement USB_RecvDataA");
}

void USB_AsyncThread(void *Unused)
{
	for(;;)
	{
		Semaphore_Wait(&glUSB_AsyncQueue, 1);
		
	}
}

