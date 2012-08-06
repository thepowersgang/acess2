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
	
	USB_int_Request(Iface->Dev, endpt, Type, Req, Value, Index, Len, Data);
}


void USB_SendData(tUSBInterface *Dev, int Endpoint, size_t Length, const void *Data)
{
	tUSBHost *host;
	tUSBEndpoint	*ep;
	void	*dest_hdl;
	ENTER("pDev iEndpoint iLength pData", Dev, Endpoint, Length, Data);

	host = Dev->Dev->Host;
	ep = &Dev->Endpoints[Endpoint-1];
	dest_hdl = Dev->Dev->EndpointHandles[ep->EndpointNum];
	LOG("dest_hdl = %p", dest_hdl);
	if( !dest_hdl ) {
		dest_hdl = host->HostDef->InitControl(host->Ptr, Dev->Dev->Address*16 + ep->EndpointNum);
		Dev->Dev->EndpointHandles[ep->EndpointNum] = dest_hdl;
		LOG("dest_hdl = %p (allocated)", dest_hdl);
	}

	Threads_ClearEvent(THREAD_EVENT_SHORTWAIT);
	host->HostDef->SendBulk(host->Ptr, dest_hdl, USB_WakeCallback, Proc_GetCurThread(), 1, (void*)Data, Length);
	Threads_WaitEvents(THREAD_EVENT_SHORTWAIT);
	
	LEAVE('-');
}

void USB_RecvData(tUSBInterface *Dev, int Endpoint, size_t Length, void *Data)
{
	tUSBHost *host;
	tUSBEndpoint	*ep;
	void	*dest_hdl;
	ENTER("pDev iEndpoint iLength pData", Dev, Endpoint, Length, Data);

	host = Dev->Dev->Host;
	ep = &Dev->Endpoints[Endpoint-1];
	dest_hdl = Dev->Dev->EndpointHandles[ep->EndpointNum];
	LOG("dest_hdl = %p", dest_hdl);
	if( !dest_hdl ) {
		dest_hdl = host->HostDef->InitControl(host->Ptr, Dev->Dev->Address*16 + ep->EndpointNum);
		Dev->Dev->EndpointHandles[ep->EndpointNum] = dest_hdl;
		LOG("dest_hdl = %p (allocated)", dest_hdl);
	}

	Threads_ClearEvent(THREAD_EVENT_SHORTWAIT);
	host->HostDef->SendBulk(host->Ptr, dest_hdl, USB_WakeCallback, Proc_GetCurThread(), 0, Data, Length);
	Threads_WaitEvents(THREAD_EVENT_SHORTWAIT);
	
	LEAVE('-');
}

void USB_RecvDataA(tUSBInterface *Dev, int Endpoint, size_t Length, void *DataBuf)
{
	tAsyncOp *op;
	tUSBHost *host;
	void	*dest_hdl;

	ENTER("pDev iEndpoint iLength pDataBuf", Dev, Endpoint, Length, DataBuf); 

	op = malloc(sizeof(*op));
	op->Next = NULL;
	op->Endpt = &Dev->Endpoints[Endpoint-1];
	op->Length = Length;
	op->Data = DataBuf;

	host = Dev->Dev->Host;
	dest_hdl = Dev->Dev->EndpointHandles[op->Endpt->EndpointNum];
	if( !dest_hdl ) {
		dest_hdl = host->HostDef->InitControl(host->Ptr, Dev->Dev->Address*16 + op->Endpt->EndpointNum);
		Dev->Dev->EndpointHandles[op->Endpt->EndpointNum] = dest_hdl;
		LOG("dest_hdl = %p (allocated)", dest_hdl);
	}
	
	LOG("IN from %p %i:%i", host->Ptr, Dev->Dev->Address, op->Endpt->EndpointNum);
	host->HostDef->SendBulk(host->Ptr, dest_hdl, USB_AsyncCallback, op, 0, DataBuf, Length);
	
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

