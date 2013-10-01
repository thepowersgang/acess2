/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * portctl.c
 * - Port control code
 */
#define DEBUG	1
#define SANITY	1
#include <acess.h>
#include "usb.h"
#include <workqueue.h>
#include <timers.h>
#include <usb_hub.h>

// === PROTOTYPES ===
void	USB_PortCtl_Init(void);
void	USB_PortCtl_Worker(void *Unused);
void	USB_PortCtl_SetPortFeature(tUSBHub *Hub, int Port, int Feat);
void	USB_PortCtl_ClearPortFeature(tUSBHub *Hub, int Port, int Feat);
 int	USB_PortCtl_GetPortStatus(tUSBHub *Hub, int Port, int Flag);

// === GLOBALS === 
tWorkqueue	gUSB_PortCtl_WorkQueue;

// === CODE ===
void USB_PortCtl_Init(void)
{
	Workqueue_Init(&gUSB_PortCtl_WorkQueue, "USB Port Reset Work Queue", offsetof(tUSBHubPort, ListNext));
	Proc_SpawnWorker(USB_PortCtl_Worker, NULL);
}

void USB_PortCtl_Worker(void *Unused)
{
	Threads_SetName("USB PortCtl Worker");
	for(;;)
	{
		tUSBHubPort *port;
		tUSBHub	*hub;
	       
		port = Workqueue_GetWork(&gUSB_PortCtl_WorkQueue);
		if( !port ) {
			Log_Warning("USB", "PortCtl Workqueue returned NULL");
			break;
		}
	       	hub = (tUSBHub*)(port - port->PortNum) - 1;

		LOG("port = %p, hub = %p", port, hub);

		switch(port->Status)
		{
		case 1:
			// Assert reset
			USB_PortCtl_SetPortFeature(hub, port->PortNum, PORT_RESET);
			LOG("Port reset starting");
			// Wait 50 ms
			Time_Delay(50);
			USB_PortCtl_ClearPortFeature(hub, port->PortNum, PORT_RESET);
			Time_Delay(10);	// May take up to 2ms for reset to clear
			// Enable port
			LOG("Port enabling");
			USB_PortCtl_SetPortFeature(hub, port->PortNum, PORT_ENABLE);
			// Begin connect processing
			LOG("Reset complete, marking connection");
			port->Status = 2;
			USB_DeviceConnected(hub, port->PortNum);
			break;
		default:
			Log_Warning("USB", "PortCtl worker: Unknown port state %i",
				port->Status);
			break;
		}
	}
}

void USB_PortCtl_BeginReset(tUSBHub *Hub, int Port)
{
	LOG("Starting %p %i", Hub, Port);
	// Set status field in hub structure
	Hub->Ports[Port].Status = 1;
	Hub->Ports[Port].PortNum = Port;
	// Add to the work queue
	Workqueue_AddWork(&gUSB_PortCtl_WorkQueue, &Hub->Ports[Port]);
	LOG("Queue added");
}

void USB_PortCtl_SetPortFeature(tUSBHub *Hub, int Port, int Feat)
{
	if( Hub->Interface->Driver == NULL ) {
		// - Host Port
		tUSBHost	*host = Hub->Interface->Dev->Host;
		ASSERT(host->HostDef->SetPortFeature);
		host->HostDef->SetPortFeature(host->Ptr, Port, Feat);
	}
	else {
		// - Hub Port
		Hub_SetPortFeature(Hub->Interface, Port, Feat);
	}
}

void USB_PortCtl_ClearPortFeature(tUSBHub *Hub, int Port, int Feat)
{
	if( Hub->Interface->Driver == NULL ) {
		// - Host Port
		tUSBHost	*host = Hub->Interface->Dev->Host;
		ASSERT(host->HostDef->ClearPortFeature);
		host->HostDef->ClearPortFeature(host->Ptr, Port, Feat);
	}
	else {
		// - Hub Port
		Hub_ClearPortFeature(Hub->Interface, Port, Feat);
	}
}

int USB_PortCtl_GetPortStatus(tUSBHub *Hub, int Port, int Flag)
{
	if( Hub->Interface->Driver == NULL ) {
		// - Host Port
		tUSBHost	*host = Hub->Interface->Dev->Host;
		ASSERT(host->HostDef->GetPortStatus);
		return host->HostDef->GetPortStatus(host->Ptr, Port, Flag);
	}
	else {
		// - Hub Port
		return Hub_GetPortStatus(Hub->Interface, Port, Flag);
	}
	return 0;
}

