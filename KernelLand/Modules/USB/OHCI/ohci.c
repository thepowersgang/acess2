/*
 * Acess2 USB Stack - OHCI Driver
 * - By John Hodge (thePowersGang)
 *
 * ohci.c
 * - Open Host Controller Interface driver
 */
#define DEBUG	0
#define VERSION	VER2(0,1)
#include <usb_host.h>
#include "ohci.h"
#include <modules.h>
#include <drv_pci.h>

// === CONSTANTS ===
#define MAX_CONTROLLERS	4
#define MAX_TD_PAGES	2
#define MAX_ENDPT_PAGES	2
#define MAX_PACKET_SIZE	0x1000	// TODO: Check what should be used

// === PROTOTYPES ===
 int	OHCI_Initialise(char **Arguments);
void	OHCI_Cleanup(void);

void	OHCI_InitialiseController(tOHCI_Controller *Controller);

void	*OHCI_int_AddTD(tOHCI_Controller *Controller, int Dest, int DataTgl, int Type, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
void	*OHCI_DataIN(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
void	*OHCI_DataOUT(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
void	*OHCI_SendSETUP(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
 int	OHCI_IsTransferComplete(void *Ptr, void *Handle);

void	*OHCI_StartPoll(void *Ptr, int Dest, int MaxPeriod, tUSBHostCb Cb, void *CbData, void *DataBuf, size_t Length);
void	OHCI_StopPoll(void *Ptr, void *Hdl);
void	OHCI_CheckPortUpdate(void *Ptr);
void	OHCI_InterruptHandler(int IRQ, void *Ptr);

tOHCI_Endpoint	*OHCI_int_AllocateEndpt(tOHCI_Controller *Controller);
tOHCI_Endpoint	*OHCI_int_GetEndptFromPhys(tPAddr PhysAddr);
tOHCI_GeneralTD	*OHCI_int_AllocateGTD(tOHCI_Controller *Controller);
tOHCI_GeneralTD	*OHCI_int_GetGTDFromPhys(tPAddr PhysAddr);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, USB_OHCI, OHCI_Initialise, OHCI_Cleanup, "USB_Core", NULL);
tUSBHostDef	gOHCI_HostDef = {
	.SendIN = OHCI_DataIN,
	.SendOUT = OHCI_DataOUT,
	.SendSETUP = OHCI_SendSETUP,
	.IsOpComplete = OHCI_IsTransferComplete,

//	.StartPolling = OHCI_StartPoll,
//	.StopPolling = OHCI_StopPoll,

	.CheckPorts = OHCI_CheckPortUpdate
	};
tOHCI_Controller	gOHCI_Controllers[MAX_CONTROLLERS];
tOHCI_GeneralTD	*gapOHCI_TDPool[MAX_TD_PAGES];	//!< Virtual pointers to TDs, each has an index in avail bits
const int	ciTDs_per_page = PAGE_SIZE/sizeof(tOHCI_GeneralTD);
tOHCI_Endpoint	*gapOHCI_EndpointPool[MAX_ENDPT_PAGES];
const int	ciEndpoints_per_page = PAGE_SIZE/sizeof(tOHCI_Endpoint);

// === CODE ===
int OHCI_Initialise(char **Arguments)
{
	 int	id = -1;
	 int	card_num = 0;
	
	while( (id = PCI_GetDeviceByClass(0x0C0310, 0xFFFFFF, id)) != -1 && card_num < MAX_CONTROLLERS )
	{
		tOHCI_Controller	*ctrlr = &gOHCI_Controllers[card_num];

		ctrlr->ID = card_num;	
		ctrlr->PciId = id;	
		ctrlr->IRQNum = PCI_GetIRQ(id);
		ctrlr->ControlSpacePhys = PCI_GetBAR(id, 0);	// Offset 0x10

		if( ctrlr->ControlSpacePhys == 0 || ctrlr->ControlSpacePhys & 1 ) {
			ctrlr->ControlSpacePhys = 0;
			continue ;
		}

		OHCI_InitialiseController( ctrlr );
	
		card_num ++;
	}

	if( id != -1 ) {
		Log_Warning("OHCI", "Too many controllers (> %i) were detected, ignoring the rest",
			MAX_CONTROLLERS);
	}
	
	return 0;
}

void OHCI_Cleanup(void)
{
	 int	i;
	// TODO: Cleanup for unload
	
	for( i = 0; i < MAX_CONTROLLERS && gOHCI_Controllers[i].ControlSpacePhys; i ++ )
	{
		// TODO: Clean up resources used
	}
}

void OHCI_InitialiseController(tOHCI_Controller *Controller)
{
	tOHCI_Controller	*cnt = Controller;
	struct sRegisters	*iospace;
	
	Log_Debug("OHCI", "Card #%i at 0x%X, IRQ %i",
		cnt->ID, cnt->ControlSpacePhys, cnt->IRQNum);
	
	// - Prepare mappings
	cnt->ControlSpace = (void*)MM_MapHWPages(cnt->ControlSpacePhys, 1);
	IRQ_AddHandler( cnt->IRQNum, OHCI_InterruptHandler, cnt);

	// Allocate HCCA area	
	cnt->HCCA = (void*)MM_AllocDMA(1, 32, &cnt->HCCAPhys);
	if( !cnt->HCCA ) {
		Log_Error("OHCI", "Unable to allocate HCCA (1 page, 32-bit)");
		return ;
	}
	// TODO: Check return value
	// - HCCA is 256 bytes in length, but I like alignment
	cnt->IntLists = (void*)( (tVAddr)cnt->HCCA + 512 );
	LOG("HCCAPhys = %P, HCCA = %p", cnt->HCCAPhys, cnt->HCCA);
	
	iospace = cnt->ControlSpace;
	
	// --- Restart the controller ---
	Uint32	fm_interval = iospace->HcFmInterval;
	iospace->HcCommandStatus |= (1 << 0);
	// - Wait 10 micro-seconds
	iospace->HcFmInterval = fm_interval;
	// (Now in UsbSuspend state)
	// - Wait 2ms

	// --- Initialise Virtual Queues ---
	memset(cnt->IntLists, 0, sizeof(*cnt->IntLists));
	// Set next pointers into a binary tree
	{
		tPAddr	next_lvl = cnt->HCCAPhys + 512;
		next_lvl += 16 * sizeof(tOHCI_Endpoint);
		for( int i = 0; i < 16; i ++ )
			cnt->IntLists->Period16[i].NextED = next_lvl + i/2 * sizeof(tOHCI_Endpoint);
		next_lvl += 8 * sizeof(tOHCI_Endpoint);
		for( int i = 0; i < 8; i ++ )
			cnt->IntLists->Period8[i].NextED = next_lvl + i/2 * sizeof(tOHCI_Endpoint);
		next_lvl += 4 * sizeof(tOHCI_Endpoint);
		for( int i = 0; i < 4; i ++ )
			cnt->IntLists->Period4[i].NextED = next_lvl + i/2 * sizeof(tOHCI_Endpoint);
		next_lvl += 2 * sizeof(tOHCI_Endpoint);
		for( int i = 0; i < 2; i ++ )
			cnt->IntLists->Period2[i].NextED = next_lvl + sizeof(tOHCI_Endpoint);
		next_lvl += 1 * sizeof(tOHCI_Endpoint);
		cnt->IntLists->Period1[0].NextED = next_lvl;
	}
	// Set all endpoints to be skipped
	for( int i = 0; i < 32; i ++ )
	{
		tOHCI_Endpoint	*ep = &cnt->IntLists->Period16[i];
		ep->Flags |= (1 << 14);	// Skip
	}

	// --- Initialise HCCA ---
	// - Interrupt table is set up to point to each 
	for( int i = 0; i < 32; i ++ )
	{
		static const int _balance[] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
		cnt->HCCA->HccaInterruptTable[i] = cnt->HCCAPhys + 512 + _balance[i&15] * sizeof(tOHCI_Endpoint);
	}
	cnt->HCCA->HccaFrameNumber = 0;
	cnt->HCCA->HccaDoneHead = 0;

	// --- Initialise Registers
	iospace->HcControlHeadED = MM_GetPhysAddr( (tVAddr)&cnt->IntLists->StopED );
	iospace->HcBulkHeadED = MM_GetPhysAddr( (tVAddr)&cnt->IntLists->StopED );
	iospace->HcHCCA = cnt->HCCAPhys;
	iospace->HcInterruptEnable = 0xFFFFFFFF;	// TODO: Without SOF detect
	iospace->HcControl = 0x3C;	// All queues on
	iospace->HcPeriodicStart = fm_interval / 10 * 9;	// 90% of fm_interval
	
	// --- Start
	iospace->HcControl |= (2 << 6);	// UsbOperational
}

// --- USB IO Functions ---
void *OHCI_int_DoTD(
	tOHCI_Controller *Controller, int Dest,
	int DataTgl, int Type,
	tUSBHostCb Cb, void *CbData,
	void *Buf, size_t Length
	)
{
	tOHCI_GeneralTD	*td;
	tPAddr	td_phys;
	tOHCI_Endpoint	*ep;

	// Sanity check
	if( Length > MAX_PACKET_SIZE )
		return NULL;
	
	// Check that the packet resides within memory the controller can access
	if( MM_GetPhysAddr((tVAddr)Buf) >= (1ULL << 32) || MM_GetPhysAddr((tVAddr)Buf + Length -1) >= (1ULL << 32) )
	{
		// TODO: Handle destination outside of 32-bit address range
		Log_Warning("OHCI", "_int_DoTD - Buffer outside of 32-bit range, TODO: Handle this");
		return NULL;
	}

	// Find Endpoint descriptor
	// TODO:
	// - Allocate one if needed
	// TODO:
	
	// Allocate a TD
	td = OHCI_int_AllocateGTD(Controller);
	if( !td ) {
		Log_Warning("OHCI", "_int_DoTD - Unable to allocate TD... oops");
		return NULL;
	}
	td_phys = MM_GetPhysAddr( (tVAddr)td );
	// Fill the TD
	td->Flags |= (1 << 18);	// Buffer rounding
	td->Flags |= (Type & 3) << 19;	// Type/Direction
	td->Flags |= (2 | DataTgl) << 24;	// Data Toggle
	td->CBP = MM_GetPhysAddr( (tVAddr) Buf );
	td->BE = MM_GetPhysAddr( (tVAddr)Buf + Length - 1 );
	// - Save callback etc
	td->CbPointer = (tVAddr)Cb;
	td->CbArg = (tVAddr)CbData;

	// Add to the end of the Endpoint's list
	if( ep->TailP )
		OHCI_int_GetGTDFromPhys( ep->TailP )->NextTD = td_phys;
	else
		ep->HeadP = td_phys;
	ep->TailP = td_phys;

	return td;
}
void *OHCI_DataIN(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	return OHCI_int_DoTD(Ptr, Dest, DataTgl, 2, Cb, CbData, Buf, Length);
}
void *OHCI_DataOUT(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	return OHCI_int_DoTD(Ptr, Dest, DataTgl, 1, Cb, CbData, Buf, Length);
}
void *OHCI_SendSETUP(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	return OHCI_int_DoTD(Ptr, Dest, DataTgl, 0, Cb, CbData, Buf, Length);
}
int OHCI_IsTransferComplete(void *Ptr, void *Handle)
{
	return 0;
}

// --- Interrupt polling ---
void *OHCI_StartPoll(void *Ptr, int Dest, int MaxPeriod, tUSBHostCb Cb, void *CbData, void *DataBuf, size_t Length)
{
	 int	slot;

	if( MaxPeriod <= 0 )	return NULL;

	// Allocate? (or obtain) an ED
	
	// Get update rate
	for( slot = 32; slot > MaxPeriod && slot; slot >>= 1 );

	// Allocate a TD
	
	// Place onto list
	switch( slot )
	{
	case 1:
		// Add to all lists
		break;
	case 2:
		// Add to every second list
		break;
	case 4:
		// Add to every fourth list
		break;
	case 8:
		// Add to every eighth list
		break;
	case 16:
		// Add to first list
		break;
	case 32:
		// Add to first list
		break;
	default:
		Log_Error("OHCI", "OHCI_StartPoll - `slot` is invalid (%i)", slot);
		break;
	}

	return NULL;
}

void OHCI_StopPoll(void *Ptr, void *Hdl)
{
	// Remove from list
}

// --- Root hub ---
void OHCI_CheckPortUpdate(void *Ptr)
{
	
}

// --- Interrupt handler
void OHCI_InterruptHandler(int IRQ, void *Ptr)
{
	tOHCI_Controller	*cnt = Ptr;
	// TODO: Interrupt handler
	Log_Debug("OHIC", "Interrupt handler on controller %i", cnt->ID);
}

// --- Internal Functions ---
tOHCI_Endpoint *OHCI_int_AllocateEndpt(tOHCI_Controller *Controller)
{
	 int	pg, i;
	// TODO: Locking
	for( pg = 0; pg < MAX_ENDPT_PAGES; pg ++ )
	{
		if( !gapOHCI_EndpointPool[pg] ) {
			gapOHCI_EndpointPool[pg] = (void*)MM_AllocDMA(1, 32, NULL);
			memset(gapOHCI_EndpointPool, 0, PAGE_SIZE);
		}
		
		for( i = 0; i < ciEndpoints_per_page; i ++ )
		{
			// Check if allocated, and if not take it
			if( gapOHCI_EndpointPool[pg][i].Flags & (1 << 31) )
				continue ;
			gapOHCI_EndpointPool[pg][i].Flags = (1 << 31);
			
			// Set controller ID
			gapOHCI_EndpointPool[pg][i].Flags |= Controller->ID << 27;
			
			return &gapOHCI_EndpointPool[pg][i];
		}
	}
	return NULL;
}

tOHCI_Endpoint *OHCI_int_GetEndptFromPhys(tPAddr PhysAddr)
{
	 int	i;
	for( i = 0; i < MAX_ENDPT_PAGES; i ++ )
	{
		tPAddr	addr;
		addr = MM_GetPhysAddr( (tVAddr)gapOHCI_EndpointPool[i] );
		if( PhysAddr >= addr && PhysAddr < addr + 0x1000 )
		{
			return gapOHCI_EndpointPool[i] + (PhysAddr - addr) / sizeof(tOHCI_Endpoint);
		}
	}
	return NULL;
}

tOHCI_GeneralTD *OHCI_int_AllocateGTD(tOHCI_Controller *Controller)
{
	// TODO: Locking
	return NULL;
}

tOHCI_GeneralTD	*OHCI_int_GetGTDFromPhys(tPAddr PhysAddr)
{
	 int	i;
	for( i = 0; i < MAX_TD_PAGES; i ++ )
	{
		tPAddr	addr;
		addr = MM_GetPhysAddr( (tVAddr)gapOHCI_TDPool[i] );
		if( PhysAddr >= addr && PhysAddr < addr + 0x1000 )
		{
			return gapOHCI_TDPool[i] + (PhysAddr - addr) / sizeof(tOHCI_GeneralTD);
		}
	}
	return NULL;
}

