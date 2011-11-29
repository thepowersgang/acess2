/*
 * Acess 2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * Universal Host Controller Interface
 */
#define DEBUG	0
#define VERSION	VER2(0,5)
#include <acess.h>
#include <vfs.h>
#include <drv_pci.h>
#include <modules.h>
#include <usb_host.h>
#include "uhci.h"

// === CONSTANTS ===
#define	MAX_CONTROLLERS	4
#define NUM_TDs	1024

// === PROTOTYPES ===
 int	UHCI_Initialise();
void	UHCI_Cleanup();
tUHCI_TD	*UHCI_int_AllocateTD(tUHCI_Controller *Cont);
void	UHCI_int_AppendTD(tUHCI_Controller *Cont, tUHCI_TD *TD);
void	*UHCI_int_SendTransaction(tUHCI_Controller *Cont, int Addr, Uint8 Type, int bTgl, tUSBHostCb Cb, void *Data, size_t Length);
void	*UHCI_DataIN(void *Ptr, int Fcn, int Endpt, int DataTgl, tUSBHostCb Cb, void *Data, size_t Length);
void	*UHCI_DataOUT(void *Ptr, int Fcn, int Endpt, int DataTgl, tUSBHostCb Cb, void *Data, size_t Length);
void	*UHCI_SendSetup(void *Ptr, int Fcn, int Endpt, int DataTgl, tUSBHostCb Cb, void *Data, size_t Length);
 int	UHCI_IsTransferComplete(void *Ptr, void *Handle);
 int	UHCI_Int_InitHost(tUHCI_Controller *Host);
void	UHCI_CheckPortUpdate(void *Ptr);
void	UHCI_InterruptHandler(int IRQ, void *Ptr);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, USB_UHCI, UHCI_Initialise, NULL, "USB_Core", NULL);
tUHCI_TD	gaUHCI_TDPool[NUM_TDs];
tUHCI_Controller	gUHCI_Controllers[MAX_CONTROLLERS];
tUSBHostDef	gUHCI_HostDef = {
	.SendIN = UHCI_DataIN,
	.SendOUT = UHCI_DataOUT,
	.SendSETUP = UHCI_SendSetup,
	.CheckPorts = UHCI_CheckPortUpdate,
	.IsOpComplete = UHCI_IsTransferComplete
	};

// === CODE ===
/**
 * \fn int UHCI_Initialise()
 * \brief Called to initialise the UHCI Driver
 */
int UHCI_Initialise(const char **Arguments)
{
	 int	i=0, id=-1;
	 int	ret;
	
	ENTER("");
	
	// Enumerate PCI Bus, getting a maximum of `MAX_CONTROLLERS` devices
	while( (id = PCI_GetDeviceByClass(0x0C03, 0xFFFF, id)) >= 0 && i < MAX_CONTROLLERS )
	{
		tUHCI_Controller	*cinfo = &gUHCI_Controllers[i];
		// NOTE: Check "protocol" from PCI?
		
		cinfo->PciId = id;
		cinfo->IOBase = PCI_GetBAR(id, 4);
		if( !(cinfo->IOBase & 1) ) {
			Log_Warning("UHCI", "MMIO is not supported");
			continue ;
		}
		cinfo->IOBase &= ~1;
		cinfo->IRQNum = PCI_GetIRQ(id);
		
		Log_Debug("UHCI", "Controller PCI #%i: IO Base = 0x%x, IRQ %i",
			id, cinfo->IOBase, cinfo->IRQNum);
		
		IRQ_AddHandler(cinfo->IRQNum, UHCI_InterruptHandler, cinfo);
	
		// Initialise Host
		ret = UHCI_Int_InitHost(&gUHCI_Controllers[i]);
		// Detect an error
		if(ret != 0) {
			LEAVE('i', ret);
			return ret;
		}
		
		cinfo->RootHub = USB_RegisterHost(&gUHCI_HostDef, cinfo, 2);
		LOG("cinfo->RootHub = %p", cinfo->RootHub);

		UHCI_CheckPortUpdate(cinfo);

		i ++;
	}
	if(i == MAX_CONTROLLERS) {
		Log_Warning("UHCI", "Over "EXPAND_STR(MAX_CONTROLLERS)" UHCI controllers detected, ignoring rest");
	}
	LEAVE('i', MODULE_ERR_OK);
	return MODULE_ERR_OK;
}

/**
 * \fn void UHCI_Cleanup()
 * \brief Called just before module is unloaded
 */
void UHCI_Cleanup()
{
}

tUHCI_TD *UHCI_int_AllocateTD(tUHCI_Controller *Cont)
{
	 int	i;
	for(i = 0; i < NUM_TDs; i ++)
	{
		if(gaUHCI_TDPool[i].Link == 0) {
			gaUHCI_TDPool[i].Link = 1;
			gaUHCI_TDPool[i].Control = 1 << 23;
			return &gaUHCI_TDPool[i];
		}
		// Still in use? Skip
		if( gaUHCI_TDPool[i].Control & (1 << 23) )
			continue ;
		// Is there a callback on it? Skip
		if( gaUHCI_TDPool[i]._info.Callback )
			continue ;
		// TODO: Garbage collect, but that means removing from the list too
		#if 0
		// Ok, this is actually unused
		gaUHCI_TDPool[i].Link = 1;
		gaUHCI_TDPool[i].Control = 1 << 23;
		return &gaUHCI_TDPool[i];
		#endif
	}
	return NULL;
}

void UHCI_int_AppendTD(tUHCI_Controller *Cont, tUHCI_TD *TD)
{
	 int	next_frame = (inw(Cont->IOBase + FRNUM) + 2) & (1024-1);
	tPAddr	td_pool_base = MM_GetPhysAddr( (tVAddr)gaUHCI_TDPool );
	tUHCI_TD	*prev_td;
	Uint32	link;

	// TODO: How to handle FRNUM incrementing while we are in this function?

	// Empty list
	if( Cont->FrameList[next_frame] & 1 )
	{
		// TODO: Ensure 32-bit paddr
		Cont->FrameList[next_frame] = MM_GetPhysAddr( (tVAddr)TD );
		LOG("next_frame = %i", next_frame);	
		return;
	}

	// Find the end of the list
	link = Cont->FrameList[next_frame];
	do {
		// TODO: Fix this to work with a non-contiguous pool
		prev_td = gaUHCI_TDPool + (link - td_pool_base) / sizeof(gaUHCI_TDPool[0]);
		link = prev_td->Link;
	} while( !(link & 1) );
	
	// Append
	prev_td->Link = MM_GetPhysAddr( (tVAddr)TD );

	LOG("next_frame = %i, prev_td = %p", next_frame, prev_td);
}

/**
 * \brief Send a transaction to the USB bus
 * \param Cont	Controller pointer
 * \param Addr	Function Address * 16 + Endpoint
 * \param bTgl	Data toggle value
 */
void *UHCI_int_SendTransaction(tUHCI_Controller *Cont, int Addr, Uint8 Type, int bTgl, tUSBHostCb Cb, void *Data, size_t Length)
{
	tUHCI_TD	*td;

	if( Length > 0x400 )	return NULL;	// Controller allows up to 0x500, but USB doesn't

	td = UHCI_int_AllocateTD(Cont);

	if( !td ) {
		// 
		Log_Error("UHCI", "No avaliable TDs, transaction dropped");
		return NULL;
	}

	td->Link = 1;
	td->Control = (Length - 1) & 0x7FF;
	td->Control |= (1 << 23);
	td->Token  = ((Length - 1) & 0x7FF) << 21;
	td->Token |= (bTgl & 1) << 19;
	td->Token |= (Addr & 0xF) << 15;
	td->Token |= ((Addr/16) & 0xFF) << 8;
	td->Token |= Type;

	// TODO: Ensure 32-bit paddr
	if( ((tVAddr)Data & (PAGE_SIZE-1)) + Length > PAGE_SIZE ) {
		Log_Warning("UHCI", "TODO: Support non single page transfers (%x + %x > %x)",
			(tVAddr)Data & (PAGE_SIZE-1), Length, PAGE_SIZE
			);
		// TODO: Need to enable IOC to copy the data back
//		td->BufferPointer = 
		return NULL;
	}
	else {
		td->BufferPointer = MM_GetPhysAddr( (tVAddr)Data );
	}

	// Interrupt on completion
	if( Cb ) {
		td->Control |= (1 << 24);
		td->_info.Callback = Cb;	// NOTE: if ERRPTR then the TD is kept allocated until checked
		if( Cb != INVLPTR )
		{
			Log_Warning("UHCI", "TODO: Support IOC... somehow");
		}
	}
	
	td->_info.DestPtr = Data;

	UHCI_int_AppendTD(Cont, td);

	return td;
}

void *UHCI_DataIN(void *Ptr, int Fcn, int Endpt, int DataTgl, tUSBHostCb Cb, void *Data, size_t Length)
{
	return UHCI_int_SendTransaction(Ptr, Fcn*16+Endpt, 0x69, DataTgl, Cb, Data, Length);
}

void *UHCI_DataOUT(void *Ptr, int Fcn, int Endpt, int DataTgl, tUSBHostCb Cb, void *Data, size_t Length)
{
	return UHCI_int_SendTransaction(Ptr, Fcn*16+Endpt, 0xE1, DataTgl, Cb, Data, Length);
}

void *UHCI_SendSetup(void *Ptr, int Fcn, int Endpt, int DataTgl, tUSBHostCb Cb, void *Data, size_t Length)
{
	return UHCI_int_SendTransaction(Ptr, Fcn*16+Endpt, 0x2D, DataTgl, Cb, Data, Length);
}

int UHCI_IsTransferComplete(void *Ptr, void *Handle)
{
	tUHCI_TD	*td = Handle;
	return !(td->Control & (1 << 23));
}

// === INTERNAL FUNCTIONS ===
/**
 * \fn int UHCI_Int_InitHost(tUCHI_Controller *Host)
 * \brief Initialises a UHCI host controller
 * \param Host	Pointer - Host to initialise
 */
int UHCI_Int_InitHost(tUHCI_Controller *Host)
{
	ENTER("pHost", Host);

	outw( Host->IOBase + USBCMD, 4 );	// GRESET
	Time_Delay(10);
	// TODO: Wait for at least 10ms
	outw( Host->IOBase + USBCMD, 0 );	// GRESET
	
	// Allocate Frame List
	// - 1 Page, 32-bit address
	// - 1 page = 1024  4 byte entries
	Host->FrameList = (void *) MM_AllocDMA(1, 32, &Host->PhysFrameList);
	if( !Host->FrameList ) {
		Log_Warning("UHCI", "Unable to allocate frame list, aborting");
		LEAVE('i', -1);
		return -1;
	}
	LOG("Allocated frame list 0x%x (0x%x)", Host->FrameList, Host->PhysFrameList);
	memsetd( Host->FrameList, 1, 1024 );	// Clear List (Disabling all entries)
	
	//! \todo Properly fill frame list
	
	// Set frame length to 1 ms
	outb( Host->IOBase + SOFMOD, 64 );
	
	// Set Frame List
	outd( Host->IOBase + FLBASEADD, Host->PhysFrameList );
	outw( Host->IOBase + FRNUM, 0 );
	
	// Enable Interrupts
	outw( Host->IOBase + USBINTR, 0x000F );
	PCI_ConfigWrite( Host->PciId, 0xC0, 2, 0x2000 );

	// Enable processing
	outw( Host->IOBase + USBCMD, 0x0001 );

	LEAVE('i', 0);
	return 0;
}

void UHCI_CheckPortUpdate(void *Ptr)
{
	tUHCI_Controller	*Host = Ptr;
	// Enable ports
	for( int i = 0; i < 2; i ++ )
	{
		 int	port = Host->IOBase + PORTSC1 + i*2;
		// Check for port change
		if( !(inw(port) & 0x0002) )	continue;
		outw(port, 0x0002);
		
		// Check if the port is connected
		if( !(inw(port) & 1) )
		{
			// Tell the USB code it's gone.
			USB_DeviceDisconnected(Host->RootHub, i);
			continue;
		}
		else
		{
			LOG("Port %i has something", i);
			// Reset port (set bit 9)
			LOG("Reset");
			outw( port, 0x0200 );
			Time_Delay(50);	// 50ms delay
			outw( port, inw(port) & ~0x0200 );
			// Enable port
			LOG("Enable");
			Time_Delay(50);	// 50ms delay
			outw( port, inw(port) | 0x0004 );
			// Tell USB there's a new device
			USB_DeviceConnected(Host->RootHub, i);
		}
	}
}

void UHCI_InterruptHandler(int IRQ, void *Ptr)
{
	tUHCI_Controller *Host = Ptr;
	Uint16	status = inw(Host->IOBase + USBSTS);
	Log_Debug("UHCI", "UHIC Interrupt, status = 0x%x", status);
	
	if( status & 1 )
	{
		// Interrupt-on-completion
	}

	outw(Host->IOBase + USBSTS, status);
}
