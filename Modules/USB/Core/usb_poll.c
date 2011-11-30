/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb_poll.c
 * - Endpoint polling
 */
#define DEBUG	1
#include <usb_core.h>
#include "usb.h"

#define POLL_ATOM	25	// 25ms atom
#define POLL_MAX	256	// Max period that can be nominated
#define POLL_SLOTS	((int)(POLL_MAX/POLL_ATOM))

// === IMPORTS ===
extern tUSBHost	*gUSB_Hosts;

// === PROTOTYPES ===
void	USB_StartPollingEndpoint(tUSBInterface *Iface, int Endpoint);

// === GLOBALS ===
tUSBEndpoint	*gUSB_PollQueues[POLL_MAX/POLL_ATOM];
 int	giUSB_PollPosition;	// Index into gUSB_PollQueues

// === CODE ===
void USB_StartPollingEndpoint(tUSBInterface *Iface, int Endpoint)
{
	tUSBEndpoint	*endpt;

	// Some sanity checks
	if(Endpoint <= 0 || Endpoint > Iface->nEndpoints)	return ;
	endpt = &Iface->Endpoints[Endpoint-1];
	if(endpt->PollingPeriod > POLL_MAX || endpt->PollingPeriod <= 0)
		return ;

	// TODO: Check that this endpoint isn't already on the queue

	endpt->InputData = malloc(endpt->MaxPacketSize);

	// Determine polling period in atoms
	endpt->PollingAtoms = (endpt->PollingPeriod + POLL_ATOM-1) / POLL_ATOM;
	if(endpt->PollingAtoms > POLL_SLOTS)	endpt->PollingAtoms = POLL_SLOTS;
	// Add to poll queue
	// TODO: Locking
	{
		 int	idx = giUSB_PollPosition + 1;
		if(idx >= POLL_SLOTS)	idx -= POLL_SLOTS;
		endpt->Next = gUSB_PollQueues[idx];
		gUSB_PollQueues[idx] = endpt;
	}
}

/**
 * \brief USB polling thread
 */
int USB_PollThread(void *unused)
{
	Threads_SetName("USB Polling Thread");
	for(;;)
	{
		tUSBEndpoint	*ep, *prev;

		if(giUSB_PollPosition == 0)
		{
			// Check hosts
			for( tUSBHost *host = gUSB_Hosts; host; host = host->Next )
			{
				host->HostDef->CheckPorts(host->Ptr);
			}
		}

		// A little evil for neater code
		prev = (void*)( (tVAddr)&gUSB_PollQueues[giUSB_PollPosition] - offsetof(tUSBEndpoint, Next) );

		// Process queue
//		LOG("giUSB_PollPosition = %i", giUSB_PollPosition);
		for( ep = gUSB_PollQueues[giUSB_PollPosition]; ep; prev = ep, ep = ep->Next )
		{
			 int	period_in_atoms = ep->PollingAtoms;
//			LOG("%i: ep = %p", giUSB_PollPosition, ep);

			// Check for invalid entries
			if(period_in_atoms < 0 || period_in_atoms > POLL_ATOM)
			{
				Log_Warning("USB", "Endpoint on polling queue with invalid period");
				continue ;
			}
			// Check for entries to delete
			if(period_in_atoms == 0)
			{
				// Remove
				prev->Next = ep->Next;
				ep->PollingAtoms = -1;	// Mark as removed
				ep = prev;	// Make sure prev is kept valid
				continue ;
			}

			// Read data
			// TODO: Check the endpoint
			// TODO: Async checking?
			// - Send the read request on all of them then wait for the first to complete
			USB_RecvDataA(
				ep->Interface, ep->EndpointIdx+1,
				ep->MaxPacketSize, ep->InputData,
				ep->Interface->Driver->Endpoints[ep->EndpointIdx].DataAvail
				);
				
			// Call callback

			// Reschedule
			if( period_in_atoms != POLL_SLOTS )
			{
				 int	newqueue_id = (giUSB_PollPosition + period_in_atoms) % POLL_SLOTS;
				tUSBEndpoint	**newqueue = &gUSB_PollQueues[newqueue_id];
				
				prev->Next = ep->Next;
				
				ep->Next = *newqueue;
				*newqueue = ep;
				ep = prev;
			}
		}
		giUSB_PollPosition ++;
		if(giUSB_PollPosition == POLL_SLOTS)
			giUSB_PollPosition = 0;
		// TODO: Check for a longer delay
		Time_Delay(POLL_ATOM);
	}
}

