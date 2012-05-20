/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb_io.c
 * - High-level IO
 */
#define DEBUG	0

#include <usb_core.h>
#include "usb.h"
#include "usb_lowlevel.h"
#include <workqueue.h>
#include <events.h>
#include "usb_async.h"

// === PROTOTYPES ===
void	USB_ReadDescriptor(tUSBInterface *Iface, int Type, int Index, int Length, void *Data);
void	USB_Request(tUSBInterface *Iface, int Endpoint, int Type, int Req, int Value, int Index, int Len, void *Data);
void	USB_SendData(tUSBInterface *Dev, int Endpoint, size_t Length, const void *Data);
void	USB_WakeCallback(void *Ptr, void *Buf, size_t Length);
void	USB_AsyncCallback(void *Ptr, void *Buf, size_t Length);
void	USB_AsyncThread(void *unused);

// === GLOBALS ===
tWorkqueue	gUSB_AsyncQueue;

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


void USB_SendData(tUSBInterface *Dev, int Endpoint, size_t Length, const void *Data)
{
	tUSBHost *host;
	tUSBEndpoint	*ep;
	ENTER("pDev iEndpoint iLength pData", Dev, Endpoint, Length, Data);

	ep = &Dev->Endpoints[Endpoint-1];
	host = Dev->Dev->Host;

	Threads_ClearEvent(THREAD_EVENT_SHORTWAIT);
	for( size_t ofs = 0; ofs < Length; ofs += ep->MaxPacketSize )
	{
		size_t	len = MIN(Length - ofs, ep->MaxPacketSize);
		
		host->HostDef->BulkOUT(
			host->Ptr, Dev->Dev->Address*16 + Dev->Endpoints[Endpoint-1].EndpointNum,
			0, (len == Length - ofs ? USB_WakeCallback : NULL), Proc_GetCurThread(),
			(char*)Data + ofs, len
			);
	}
	Threads_WaitEvents(THREAD_EVENT_SHORTWAIT);
	
	LEAVE('-');
}

void USB_RecvData(tUSBInterface *Dev, int Endpoint, size_t Length, void *Data)
{
	tUSBHost *host;
	tUSBEndpoint	*ep;
	ENTER("pDev iEndpoint iLength pData", Dev, Endpoint, Length, Data);

	ep = &Dev->Endpoints[Endpoint-1];
	host = Dev->Dev->Host;

	Threads_ClearEvent(THREAD_EVENT_SHORTWAIT);
	for( size_t ofs = 0; ofs < Length; ofs += ep->MaxPacketSize )
	{
		size_t	len = MIN(Length - ofs, ep->MaxPacketSize);
		
		host->HostDef->BulkIN(
			host->Ptr, Dev->Dev->Address*16 + Dev->Endpoints[Endpoint-1].EndpointNum,
			0, (len == Length - ofs ? USB_WakeCallback : NULL), Proc_GetCurThread(),
			(char*)Data + ofs, len
			);
	}
	Threads_WaitEvents(THREAD_EVENT_SHORTWAIT);
	
	LEAVE('-');
}

void USB_RecvDataA(tUSBInterface *Dev, int Endpoint, size_t Length, void *DataBuf, tUSB_DataCallback Callback)
{
	tAsyncOp *op;
	tUSBHost *host;

	ENTER("pDev iEndpoint iLength pDataBuf", Dev, Endpoint, Length, DataBuf); 

	op = malloc(sizeof(*op));
	op->Next = NULL;
	op->Endpt = &Dev->Endpoints[Endpoint-1];
	op->Length = Length;
	op->Data = DataBuf;

	// TODO: Handle transfers that are larger than one packet
	// TODO: Data toggle

	host = Dev->Dev->Host;
	
	LOG("IN from %p %i:%i", host->Ptr, Dev->Dev->Address, op->Endpt->EndpointNum);
	for( size_t ofs = 0; ofs < Length; ofs += op->Endpt->MaxPacketSize )
	{
		size_t	len = MIN(Length - ofs, op->Endpt->MaxPacketSize);
		
		host->HostDef->BulkIN(
			host->Ptr, Dev->Dev->Address*16 + op->Endpt->EndpointNum,
			0, (len == Length - ofs ? USB_AsyncCallback : NULL), op,
			(char*)DataBuf + ofs, len
			);
	}
	
	LEAVE('-');

//	Log_Warning("USB", "TODO: Implement USB_RecvDataA");
}

void USB_WakeCallback(void *Ptr, void *Buf, size_t Length)
{
	Threads_PostEvent(Ptr, THREAD_EVENT_SHORTWAIT);
}

void USB_AsyncCallback(void *Ptr, void *Buf, size_t Length)
{
	tAsyncOp *op = Ptr;
	op->Length = Length;
	LOG("adding %p to work queue", op);
	Workqueue_AddWork(&gUSB_AsyncQueue, op);
}

void USB_AsyncThread(void *Unused)
{
	Threads_SetName("USB Async IO Thread");
	for(;;)
	{
		tAsyncOp *op = Workqueue_GetWork(&gUSB_AsyncQueue);
		tUSBInterface	*iface = op->Endpt->Interface;

		LOG("op = %p", op);	

		iface->Driver->Endpoints[op->Endpt->EndpointIdx].DataAvail(
			iface, op->Endpt->EndpointIdx,
			op->Length, op->Data);
		
		free(op);
	}
}

