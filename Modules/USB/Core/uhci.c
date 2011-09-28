/*
 * Acess 2 USB Stack
 * Universal Host Controller Interface
 */
#define DEBUG	1
#include <acess.h>
#include <vfs.h>
#include <drv_pci.h>
#include "usb.h"
#include "uhci.h"

// === CONSTANTS ===
#define	MAX_CONTROLLERS	4
#define NUM_TDs	1024

// === PROTOTYPES ===
 int	UHCI_Initialise();
void	UHCI_Cleanup();
tUHCI_TD	*UHCI_int_AllocateTD(tUHCI_Controller *Cont);
void	UHCI_int_AppendTD(tUHCI_Controller *Cont, tUHCI_TD *TD);
 int	UHCI_int_SendTransaction(tUHCI_Controller *Cont, int Fcn, int Endpt, int DataTgl, Uint8 Type, void *Data, size_t Length);
 int	UHCI_DataIN(void *Ptr, int Fcn, int Endpt, int DataTgl, void *Data, size_t Length);
 int	UHCI_DataOUT(void *Ptr, int Fcn, int Endpt, int DataTgl, void *Data, size_t Length);
 int	UHCI_SendSetup(void *Ptr, int Fcn, int Endpt, int DataTgl, void *Data, size_t Length);
 int	UHCI_Int_InitHost(tUHCI_Controller *Host);
void	UHCI_InterruptHandler(int IRQ);

// === GLOBALS ===
tUHCI_TD	gaUHCI_TDPool[NUM_TDs];
tUHCI_Controller	gUHCI_Controllers[MAX_CONTROLLERS];
tUSBHost	gUHCI_HostDef = {
	.SendIN = UHCI_DataIN,
	.SendOUT = UHCI_DataOUT,
	.SendSETUP = UHCI_SendSetup,
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
		// Assign a port range (BAR4, Reserve 32 ports)
		cinfo->IOBase = PCI_GetBAR(id, 4);
		if( !(cinfo->IOBase & 1) ) {
			Log_Warning("UHCI", "MMIO is not supported");
			continue ;
		}
		cinfo->IRQNum = PCI_GetIRQ(id);
		
		Log_Debug("UHCI", "Controller PCI #%i: IO Base = 0x%x, IRQ %i",
			id, cinfo->IOBase, cinfo->IRQNum);
		
		IRQ_AddHandler(cinfo->IRQNum, UHCI_InterruptHandler);
	
		// Initialise Host
		ret = UHCI_Int_InitHost(&gUHCI_Controllers[i]);
		// Detect an error
		if(ret != 0) {
			LEAVE('i', ret);
			return ret;
		}
		
		USB_RegisterHost(&gUHCI_HostDef, cinfo);

		i ++;
	}
	if(i == MAX_CONTROLLERS) {
		Log_Warning("UHCI", "Over "EXPAND_STR(MAX_CONTROLLERS)" UHCI controllers detected, ignoring rest");
	}
	LEAVE('i', i);
	return i;
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
			return &gaUHCI_TDPool[i];
		}
	}
	return NULL;
}

void UHCI_int_AppendTD(tUHCI_Controller *Cont, tUHCI_TD *TD)
{
	
}

/**
 * \brief Send a transaction to the USB bus
 * \param ControllerID Controller
 * \param Fcn	Function Address
 * \param Endpt	Endpoint
 */
int UHCI_int_SendTransaction(tUHCI_Controller *Cont, int Fcn, int Endpt, int DataTgl, Uint8 Type, void *Data, size_t Length)
{
	tUHCI_TD	*td;

	if( Length > 0x400 )	return -1;	// Controller allows up to 0x500, but USB doesn't

	td = UHCI_int_AllocateTD(Cont);

	td->Link = 1;
	td->Control = (Length - 1) & 0x7FF;
	td->Token  = ((Length - 1) & 0x7FF) << 21;
	td->Token |= (DataTgl & 1) << 19;
	td->Token |= (Endpt & 0xF) << 15;
	td->Token |= (Fcn & 0xFF) << 8;
	td->Token |= Type;

	// TODO: Ensure 32-bit paddr
	if( ((tVAddr)Data & PAGE_SIZE) + Length > PAGE_SIZE ) {
		Log_Warning("UHCI", "TODO: Support non single page transfers");
//		td->BufferPointer = 
		return 1;
	}
	else {
		td->BufferPointer = MM_GetPhysAddr( (tVAddr)Data );
	}

	UHCI_int_AppendTD(Cont, td);

	// Wait until done, then return
	while(td->Link != 0)
		Threads_Yield();
	return 0;
}

int UHCI_DataIN(void *Ptr, int Fcn, int Endpt, int DataTgl, void *Data, size_t Length)
{
	return UHCI_int_SendTransaction(Ptr, Fcn, Endpt, DataTgl, 0x69, Data, Length);
}

int UHCI_DataOUT(void *Ptr, int Fcn, int Endpt, int DataTgl, void *Data, size_t Length)
{
	return UHCI_int_SendTransaction(Ptr, Fcn, Endpt, DataTgl, 0xE1, Data, Length);
}

int UHCI_SendSetup(void *Ptr, int Fcn, int Endpt, int DataTgl, void *Data, size_t Length)
{
	return UHCI_int_SendTransaction(Ptr, Fcn, Endpt, DataTgl, 0x2D, Data, Length);
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
	
	// Set Frame List Address
	outd( Host->IOBase + FLBASEADD, Host->PhysFrameList );
	
	// Set Frame Number
	outw( Host->IOBase + FRNUM, 0 );
	
	// Enable Interrupts
	//PCI_WriteWord( Host->PciId, 0xC0, 0x2000 );
	
	LEAVE('i', 0);
	return 0;
}
