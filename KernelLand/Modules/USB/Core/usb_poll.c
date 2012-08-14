/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb_poll.c
 * - Endpoint polling
 */
#define DEBUG	0
#include <usb_core.h>
#include "usb.h"
#include <timers.h>
#include "usb_async.h"

// === IMPORTS ===
extern tUSBHost	*gUSB_Hosts;

// === PROTOTYPES ===
void	USB_StartPollingEndpoint(tUSBInterface *Iface, int Endpoint);

// === GLOBALS ===

// === CODE ===
void USB_int_PollCallback(void *Ptr, void *Data, size_t Length)
{
	tAsyncOp *op;
	tUSBEndpoint	*ep = Ptr;
	
	op = malloc(sizeof(*op));

	op->Next = NULL;
	op->Endpt = ep;
	op->Length = Length;
	op->Data = ep->InputData;

	LOG("op %p, endpoint %p (0x%x)", op, ep,
		ep->Interface->Dev->Address * 16 + ep->EndpointNum);

	Workqueue_AddWork(&gUSB_AsyncQueue, op);
}

void USB_StartPollingEndpoint(tUSBInterface *Iface, int Endpoint)
{
	tUSBEndpoint	*endpt;

	ENTER("pIface iEndpoint", Iface, Endpoint);

	// Some sanity checks
	if(Endpoint <= 0 || Endpoint > Iface->nEndpoints) {
		LEAVE('-');
		return ;
	}
	endpt = &Iface->Endpoints[Endpoint-1];
	LOG("endpt(%p)->PollingPeriod = %i", endpt, endpt->PollingPeriod);
	if(endpt->PollingPeriod > 256 || endpt->PollingPeriod <= 0) {
		LOG("Invalid polling period");
		LEAVE('-');
		return ;
	}

	// TODO: Check that this endpoint isn't already on the queue

	endpt->InputData = malloc(endpt->MaxPacketSize);
	LOG("Polling 0x%x at %i ms", Iface->Dev->Address * 16 + endpt->EndpointNum, endpt->PollingPeriod);
	Iface->Dev->Host->HostDef->InitInterrupt(
		Iface->Dev->Host->Ptr, Iface->Dev->Address * 16 + endpt->EndpointNum,
		0, endpt->PollingPeriod,
		USB_int_PollCallback, endpt,
		endpt->InputData, endpt->MaxPacketSize
		);
	LEAVE('-');
}

/**
 * \brief USB polling thread
 */
int USB_PollThread(void *unused)
{
	Threads_SetName("USB Polling Thread");
	for(;;)
	{
		// Check hosts
		for( tUSBHost *host = gUSB_Hosts; host; host = host->Next )
		{
			host->HostDef->CheckPorts(host->Ptr);
		}

		Time_Delay(100);
	}
}

