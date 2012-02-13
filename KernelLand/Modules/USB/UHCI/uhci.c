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
#include <timers.h>

// === CONSTANTS ===
#define	MAX_CONTROLLERS	4
#define NUM_TDs	1024

// === PROTOTYPES ===
 int	UHCI_Initialise(char **Arguments);
void	UHCI_Cleanup();
tUHCI_TD	*UHCI_int_AllocateTD(tUHCI_Controller *Cont);
void	UHCI_int_AppendTD(tUHCI_Controller *Cont, tUHCI_TD *TD);
void	*UHCI_int_SendTransaction(tUHCI_Controller *Cont, int Addr, Uint8 Type, int bTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
void	*UHCI_DataIN(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
void	*UHCI_DataOUT(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData,  void *Buf, size_t Length);
void	*UHCI_SendSetup(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
 int	UHCI_IsTransferComplete(void *Ptr, void *Handle);
 int	UHCI_Int_InitHost(tUHCI_Controller *Host);
void	UHCI_CheckPortUpdate(void *Ptr);
void	UHCI_InterruptHandler(int IRQ, void *Ptr);
// 
static void	_OutByte(tUHCI_Controller *Host, int Reg, Uint8 Value);
static void	_OutWord(tUHCI_Controller *Host, int Reg, Uint16 Value);
static void	_OutDWord(tUHCI_Controller *Host, int Reg, Uint32 Value);
static Uint16	_InWord(tUHCI_Controller *Host, int Reg);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, USB_UHCI, UHCI_Initialise, NULL, "USB_Core", NULL);
tUHCI_TD	gaUHCI_TDPool[NUM_TDs];
tUHCI_Controller	gUHCI_Controllers[MAX_CONTROLLERS];
tUSBHostDef	gUHCI_HostDef = {
	.SendIN = UHCI_DataIN,
	.SendOUT = UHCI_DataOUT,
	.SendSETUP = UHCI_SendSetup,
	.IsOpComplete = UHCI_IsTransferComplete,
	.CheckPorts = UHCI_CheckPortUpdate
	};

// === CODE ===
/**
 * \fn int UHCI_Initialise()
 * \brief Called to initialise the UHCI Driver
 */
int UHCI_Initialise(char **Arguments)
{
	 int	i=0, id=-1;
	 int	ret;
	
	ENTER("");
	
	// Enumerate PCI Bus, getting a maximum of `MAX_CONTROLLERS` devices
	while( (id = PCI_GetDeviceByClass(0x0C0300, 0xFFFFFF, id)) >= 0 && i < MAX_CONTROLLERS )
	{
		tUHCI_Controller	*cinfo = &gUHCI_Controllers[i];
		Uint32	base_addr;
		// NOTE: Check "protocol" from PCI?
		
		cinfo->PciId = id;
		base_addr = PCI_GetBAR(id, 4);
		
		if( base_addr & 1 )
		{
			cinfo->IOBase = base_addr & ~1;
			cinfo->MemIOMap = NULL;
		}
		else
		{
			cinfo->MemIOMap = (void*)MM_MapHWPages(base_addr, 1);
		}
		cinfo->IRQNum = PCI_GetIRQ(id);
		
		Log_Debug("UHCI", "Controller PCI #%i: IO Base = 0x%x, IRQ %i",
			id, base_addr, cinfo->IRQNum);
		
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

		i ++;
	}

	if(i == 0) {
		LEAVE('i', MODULE_ERR_NOTNEEDED);
		return MODULE_ERR_NOTNEEDED;
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

tUHCI_TD *UHCI_int_GetTDFromPhys(tPAddr PAddr)
{
	// TODO: Fix this to work with a non-contiguous pool
	static tPAddr	td_pool_base;
	const int pool_size = NUM_TDs;
	 int	offset;
	if(!td_pool_base)	td_pool_base = MM_GetPhysAddr( (tVAddr)gaUHCI_TDPool );
	offset = (PAddr - td_pool_base) / sizeof(gaUHCI_TDPool[0]);
	if( offset < 0 || offset >= pool_size )
	{
		Log_Error("UHCI", "TD PAddr %P not from pool", PAddr);
		return NULL;
	}
	return gaUHCI_TDPool + offset;
}

void UHCI_int_AppendTD(tUHCI_Controller *Cont, tUHCI_TD *TD)
{
	 int	next_frame = (_InWord(Cont, FRNUM) + 2) & (1024-1);
	tUHCI_TD	*prev_td;
	Uint32	link;

	// TODO: How to handle FRNUM incrementing while we are in this function?

	// Empty list
	if( Cont->FrameList[next_frame] & 1 )
	{
		// TODO: Ensure 32-bit paddr
		Cont->FrameList[next_frame] = MM_GetPhysAddr( (tVAddr)TD );
		TD->Control |= (1 << 24);	// Ensure that there is an interrupt for each used frame
		LOG("next_frame = %i", next_frame);	
		return;
	}

	// Find the end of the list
	link = Cont->FrameList[next_frame];
	do {
		prev_td = UHCI_int_GetTDFromPhys(link);
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
void *UHCI_int_SendTransaction(
	tUHCI_Controller *Cont, int Addr, Uint8 Type, int bTgl,
	tUSBHostCb Cb, void *CbData, void *Data, size_t Length)
{
	tUHCI_TD	*td;

	if( Length > 0x400 )	return NULL;	// Controller allows up to 0x500, but USB doesn't

	td = UHCI_int_AllocateTD(Cont);

	if( !td ) {
		// TODO: Wait for one to free?
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
		td->_info.bCopyData = 1;
		return NULL;
	}
	else {
		td->BufferPointer = MM_GetPhysAddr( (tVAddr)Data );
		td->_info.bCopyData = 0;
	}

	// Interrupt on completion
	if( Cb ) {
		td->Control |= (1 << 24);
		LOG("IOC Cb=%p CbData=%p", Cb, CbData);
		td->_info.Callback = Cb;	// NOTE: if ERRPTR then the TD is kept allocated until checked
		td->_info.CallbackPtr = CbData;
	}
	
	td->_info.DataPtr = Data;

	UHCI_int_AppendTD(Cont, td);

	return td;
}

void *UHCI_DataIN(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	return UHCI_int_SendTransaction(Ptr, Dest, 0x69, DataTgl, Cb, CbData, Buf, Length);
}

void *UHCI_DataOUT(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	return UHCI_int_SendTransaction(Ptr, Dest, 0xE1, DataTgl, Cb, CbData, Buf, Length);
}

void *UHCI_SendSetup(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	return UHCI_int_SendTransaction(Ptr, Dest, 0x2D, DataTgl, Cb, CbData, Buf, Length);
}

int UHCI_IsTransferComplete(void *Ptr, void *Handle)
{
	tUHCI_TD	*td = Handle;
	 int	ret;
	ret = !(td->Control & (1 << 23));
	if(ret) {
		td->_info.Callback = NULL;
		td->Link = 0;
	}
	return ret;
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

	_OutWord( Host, USBCMD, 4 );	// GRESET
	// TODO: Wait for at least 10ms
	_OutWord( Host, USBCMD, 0 );	// GRESET
	
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
	_OutByte( Host, SOFMOD, 64 );
	
	// Set Frame List
	_OutDWord( Host, FLBASEADD, Host->PhysFrameList );
	_OutWord( Host, FRNUM, 0 );
	
	// Enable Interrupts
	_OutWord( Host, USBINTR, 0x000F );
	PCI_ConfigWrite( Host->PciId, 0xC0, 2, 0x2000 );

	// Enable processing
	_OutWord( Host, USBCMD, 0x0001 );

	LEAVE('i', 0);
	return 0;
}

void UHCI_CheckPortUpdate(void *Ptr)
{
	tUHCI_Controller	*Host = Ptr;
	// Enable ports
	for( int i = 0; i < 2; i ++ )
	{
		 int	port = PORTSC1 + i*2;
		Uint16	status;
	
		status = _InWord(Host, port);
		// Check for port change
		if( !(status & 0x0002) )	continue;
		_OutWord(Host, port, 0x0002);
		
		// Check if the port is connected
		if( !(status & 1) )
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
			_OutWord(Host, port, 0x0200);
			Time_Delay(50);	// 50ms delay
			_OutWord(Host, port, _InWord(Host, port) & ~0x0200);
			// Enable port
			LOG("Enable");
			Time_Delay(50);	// 50ms delay
			_OutWord(Host, port, _InWord(Host, port) | 0x0004);
			// Tell USB there's a new device
			USB_DeviceConnected(Host->RootHub, i);
		}
	}
}

void UHCI_InterruptHandler(int IRQ, void *Ptr)
{
	tUHCI_Controller *Host = Ptr;
	 int	frame = (_InWord(Host, FRNUM) - 1) & 0x3FF;
	Uint16	status = _InWord(Host, USBSTS);
//	Log_Debug("UHCI", "UHIC Interrupt, status = 0x%x, frame = %i", status, frame);
	
	// Interrupt-on-completion
	if( status & 1 )
	{
		tPAddr	link;
		
		for( int i = 0; i < 10; i ++ )
		{
			link = Host->FrameList[frame];
			Host->FrameList[frame] = 1;
			while( link && !(link & 1) )
			{
				tUHCI_TD *td = UHCI_int_GetTDFromPhys(link);
				 int	byte_count = (td->Control&0x7FF)+1;
				LOG("link = 0x%x, td = %p, byte_count = %i", link, td, byte_count);
				// Handle non-page aligned destination
				// TODO: This will break if the destination is not in global memory
				if(td->_info.bCopyData)
				{
					void *ptr = (void*)MM_MapTemp(td->BufferPointer);
					Log_Debug("UHCI", "td->_info.DataPtr = %p", td->_info.DataPtr);
					memcpy(td->_info.DataPtr, ptr, byte_count);
					MM_FreeTemp((tVAddr)ptr);
				}
				// Callback
				if(td->_info.Callback && td->_info.Callback != INVLPTR)
				{
					LOG("Calling cb %p", td->_info.Callback);
					td->_info.Callback(td->_info.CallbackPtr, td->_info.DataPtr, byte_count);
					td->_info.Callback = NULL;
				}
				link = td->Link;
				if( td->_info.Callback != INVLPTR )
					td->Link = 0;
			}
			
			if(frame == 0)
				frame = 0x3ff;
			else
				frame --;
		}
		
//		Host->LastCleanedFrame = frame;
	}

	LOG("status = 0x%02x", status);
	_OutWord(Host, USBSTS, status);
}

void _OutByte(tUHCI_Controller *Host, int Reg, Uint8 Value)
{
	if( Host->MemIOMap )
		((Uint8*)Host->MemIOMap)[Reg] = Value;
	else
		outb(Host->IOBase + Reg, Value);
}

void _OutWord(tUHCI_Controller *Host, int Reg, Uint16 Value)
{
	if( Host->MemIOMap )
		Host->MemIOMap[Reg/2] = Value;
	else
		outw(Host->IOBase + Reg, Value);
}

void _OutDWord(tUHCI_Controller *Host, int Reg, Uint32 Value)
{
	if( Host->MemIOMap )
		((Uint32*)Host->MemIOMap)[Reg/4] = Value;
	else
		outd(Host->IOBase + Reg, Value);
}

Uint16 _InWord(tUHCI_Controller *Host, int Reg)
{
	if( Host->MemIOMap )
		return Host->MemIOMap[Reg/2];
	else
		return inw(Host->IOBase + Reg);
}

