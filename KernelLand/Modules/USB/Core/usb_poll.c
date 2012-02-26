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

#define POLL_ATOM	25	// 25ms atom
#define POLL_MAX	256	// Max period that can be nominated
#define POLL_SLOTS	((int)(POLL_MAX/POLL_ATOM))

// === IMPORTS ===
extern tUSBHost	*gUSB_Hosts;

// === PROTOTYPES ===
void	USB_StartPollingEndpoint(tUSBInterface *Iface, int Endpoint);

// === GLOBALS ===

// === CODE ===
void USB_int_PollCallback(void *Ptr, void *Data, size_t Length)
{
	tUSBEndpoint	*ep = Ptr;
	
	ep->Interface->Driver->Endpoints[ep->EndpointIdx].DataAvail(ep->Interface, ep->EndpointIdx, Length, Data);
}

void USB_StartPollingEndpoint(tUSBInterface *Iface, int Endpoint)
{
	tUSBEndpoint	*endpt;

	ENTER("pIface iEndpoint", Iface, Endpoint);
	LEAVE('-');

	// Some sanity checks
	if(Endpoint <= 0 || Endpoint > Iface->nEndpoints)
		return ;
	endpt = &Iface->Endpoints[Endpoint-1];
	LOG("endpt(%p)->PollingPeriod = %i", endpt, endpt->PollingPeriod);
	if(endpt->PollingPeriod > POLL_MAX || endpt->PollingPeriod <= 0)
		return ;

	// TODO: Check that this endpoint isn't already on the queue

	endpt->InputData = malloc(endpt->MaxPacketSize);

	Iface->Dev->Host->HostDef->InterruptIN(
		Iface->Dev->Host->Ptr,
		Iface->Dev->Address * 16 + endpt->EndpointNum,
		endpt->PollingPeriod,
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

		Time_Delay(POLL_ATOM);
	}
}

