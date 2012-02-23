/*
 * Acess 2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * Universal Host Controller Interface
 */
#define DEBUG	1
#define VERSION	VER2(0,5)
#include <acess.h>
#include <vfs.h>
#include <drv_pci.h>
#include <modules.h>
#include <usb_host.h>
#include "uhci.h"
#include <timers.h>
#include <semaphore.h>

// === CONSTANTS ===
#define	MAX_CONTROLLERS	4
//#define NUM_TDs	1024
#define NUM_TDs	64

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
void	UHCI_int_InterruptThread(void *Unused);
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
tSemaphore	gUHCI_InterruptSempahore;

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
	
	// Initialise with no maximum value
	Semaphore_Init( &gUHCI_InterruptSempahore, 0, 0, "UHCI", "Interrupt Queue");

	if( PCI_GetDeviceByClass(0x0C0300, 0xFFFFFF, -1) < 0 )
	{
		LEAVE('i', MODULE_ERR_NOTNEEDED);
		return MODULE_ERR_NOTNEEDED;
	}
	
	// Spin off interrupt handling thread
	Proc_SpawnWorker( UHCI_int_InterruptThread, NULL );

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
	static tMutex	lock;
	Mutex_Acquire( &lock );
	for( int i = 0; i < NUM_TDs; i ++ )
	{
		if(gaUHCI_TDPool[i]._info.bActive == 0)
		{
			gaUHCI_TDPool[i].Link = 1;
			gaUHCI_TDPool[i].Control = (1 << 23);
			gaUHCI_TDPool[i]._info.bActive = 1;
			gaUHCI_TDPool[i]._info.bComplete = 0;
			Mutex_Release( &lock );
			return &gaUHCI_TDPool[i];
		}
	}
	Mutex_Release( &lock );
	return NULL;
}

void UHCI_int_AppendTD(tUHCI_Controller *Cont, tUHCI_TD *TD)
{
	static tMutex	lock;	// TODO: Should I use a shortlock (avoid being preempted)

	Mutex_Acquire(&lock);
	
	#if 0
	 int	next_frame;

	next_frame = (_InWord(Cont, FRNUM) + 2) & (1024-1);

	TD->Control |= (1 << 24);	// Ensure that there is an interrupt for each used frame
	
	TD->Link = Cont->FrameList[next_frame];
	Cont->FrameList[next_frame] = MM_GetPhysAddr( (tVAddr)TD );
	#else

	// TODO: Support other QHs
	tUHCI_QH *qh = &Cont->BulkQH;
	
	// Ensure that there is an interrupt for each used frame
	TD->Control |= (1 << 24);

	// Stop controller
	_OutWord( Cont, USBCMD, 0x0000 );
	
	// Add
	TD->Link = 1;
	if( qh->Child & 1 ) {
		qh->Child = MM_GetPhysAddr( (tVAddr)TD );
	}
	else {
		qh->_LastItem->Link = MM_GetPhysAddr( (tVAddr)TD );
	}
	qh->_LastItem = TD;

	// Reenable controller
	_OutWord( Cont, USBCMD, 0x0001 );
	#endif

	Mutex_Release(&lock);
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
	tUHCI_ExtraTDInfo	*info = NULL;

	if( Length > 0x400 ) {
		Log_Error("UHCI", "Transaction length too large (%i > 0x400)", Length);
		return NULL;	// Controller allows up to 0x500, but USB doesn't
	}

	td = UHCI_int_AllocateTD(Cont);

	if( !td ) {
		// TODO: Wait for one to free?
		Log_Error("UHCI", "No avaliable TDs, transaction dropped");
		return NULL;
	}

	LOG("TD %p %i bytes, Type %x to 0x%x",
		td, Length, Type, Addr);

	td->Control = (Length - 1) & 0x7FF;
	td->Control |= (1 << 23);
	td->Token  = ((Length - 1) & 0x7FF) << 21;
	td->Token |= (bTgl & 1) << 19;
	td->Token |= (Addr & 0xF) << 15;
	td->Token |= ((Addr/16) & 0xFF) << 8;
	td->Token |= Type;

	if(
		((tVAddr)Data & (PAGE_SIZE-1)) + Length > PAGE_SIZE
	#if PHYS_BITS > 32
		|| MM_GetPhysAddr( (tVAddr)Data ) >> 32
	#endif
		)
	{
		td->BufferPointer = MM_AllocPhysRange(1, 32);

		LOG("Allocated page %x", td->BufferPointer);		

		if( Type == 0x69 )	// IN token
		{
			LOG("Relocated IN");
			info = calloc( sizeof(tUHCI_ExtraTDInfo), 1 );
			info->Offset = ((tVAddr)Data & (PAGE_SIZE-1));
			info->FirstPage = MM_GetPhysAddr( (tVAddr)Data );
			info->SecondPage = MM_GetPhysAddr( (tVAddr)Data + Length - 1 );
		}
		else
		{
			LOG("Relocated OUT/SETUP");
			tVAddr	ptr = MM_MapTemp(td->BufferPointer);
			memcpy( (void*)ptr, Data, Length );
			MM_FreeTemp(ptr);
			td->Control |= (1 << 24);
		}
		td->_info.bFreePointer = 1;
	}
	else
	{
		td->BufferPointer = MM_GetPhysAddr( (tVAddr)Data );
		td->_info.bFreePointer = 0;
	}

	// Interrupt on completion
	if( Cb )
	{
		if( !info )
			info = calloc( sizeof(tUHCI_ExtraTDInfo), 1 );
		LOG("IOC Cb=%p CbData=%p", Cb, CbData);
		// NOTE: if ERRPTR then the TD is kept allocated until checked
		info->Callback = Cb;
		info->CallbackPtr = CbData;
	}
	
	if( info ) {
		LOG("info = %p", info);
		td->Control |= (1 << 24);
		td->_info.ExtraInfo = info;
	}

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
	#if DEBUG
	tUHCI_Controller	*Cont = &gUHCI_Controllers[0];
	LOG("%p->Control = %x", td, td->Control);
	LOG("USBSTS = 0x%x, USBINTR = 0x%x", _InWord(Cont, USBSTS), _InWord(Cont, USBINTR));
	LOG("Cont->BulkQH.Child = %x", Cont->BulkQH.Child);
	#endif
	if(td->Control & (1 << 23)) {
		return 0;
	}
//	LOG("inactive, waiting for completion");
	if(td->_info.bComplete)
	{
		td->_info.bActive = 0;
		td->_info.bComplete = 0;
		td->Link = 0;
		return 1;
	}
	else
	{
		return 0;
	}
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
	Time_Delay(10);
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

	// TODO: Handle QHs not being in a 32-bit paddr range
	// Need another page, probably get some more TDs from it too

	// Set up interrupt and bulk queue
	Host->InterruptQH.Next = MM_GetPhysAddr( (tVAddr)&Host->ControlQH ) | 2;
	Host->InterruptQH.Child = 1;
	Host->ControlQH.Next = MM_GetPhysAddr( (tVAddr)&Host->BulkQH ) | 2;
	Host->ControlQH.Child = 1;
	Host->BulkQH.Next = 1;
	Host->BulkQH.Child = 1;

	LOG("Allocated frame list 0x%x (0x%x)", Host->FrameList, Host->PhysFrameList);
	for( int i = 0; i < 1024; i ++ )
		Host->FrameList[i] = MM_GetPhysAddr( (tVAddr)&Host->InterruptQH ) | 2;
	
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

void UHCI_int_InterruptThread(void *Unused)
{
	Threads_SetName("UHCI Interrupt Handler");
	for( ;; )
	{
		LOG("zzzzz....");
		// 0 = Take all
		Semaphore_Wait(&gUHCI_InterruptSempahore, 0);
		LOG("Huh?");
	
		for( int i = 0; i < NUM_TDs; i ++ )
		{
			 int	byte_count;
			tUHCI_ExtraTDInfo	*info;
			tUHCI_TD	*td;
			
			td = &gaUHCI_TDPool[i];
			info = td->_info.ExtraInfo;

			// Skip completely inactive TDs
			if( td->_info.bActive == 0 )	continue ;
			// Skip ones that are still in use
			if( td->Control & (1 << 23) )	continue ;

			// If no callback/alt buffer, mark as free and move on
			if( td->_info.ExtraInfo )
			{
				// Get size of transfer
				byte_count = (td->Control & 0x7FF)+1;
			
				// Handle non page-aligned destination (with a > 32-bit paddr)
				if(info->FirstPage)
				{
					char	*src, *dest;
					 int	src_ofs = td->BufferPointer & (PAGE_SIZE-1);
					src = (void *) MM_MapTemp(td->BufferPointer);
					dest = (void *) MM_MapTemp(info->FirstPage);
					// Check for a single page transfer
					if( byte_count + info->Offset <= PAGE_SIZE )
					{
						LOG("Single page copy %P to %P of %p",
							td->BufferPointer, info->FirstPage, td);
						memcpy(dest + info->Offset, src + src_ofs, byte_count);
					}
					else
					{
						// Multi-page
						LOG("Multi page copy %P to (%P,%P) of %p",
							td->BufferPointer, info->FirstPage, info->SecondPage, td);
						 int	part_len = PAGE_SIZE - info->Offset;
						memcpy(dest + info->Offset, src + src_ofs, part_len);
						MM_FreeTemp( (tVAddr)dest );
						dest = (void *) MM_MapTemp(info->SecondPage);
						memcpy(dest, src + src_ofs + part_len, byte_count - part_len);
					}
					MM_FreeTemp( (tVAddr)src );
					MM_FreeTemp( (tVAddr)dest );
				}
	
				// Don't mark as inactive, the check should do that
				if( info->Callback == INVLPTR )
				{
					LOG("Marking %p as complete", td);
					td->_info.bComplete = 1;
					free( info );
					td->_info.ExtraInfo = NULL;
					if( td->_info.bFreePointer )
						MM_DerefPhys( td->BufferPointer );			
					continue ;
				}

				// Callback
				if( info->Callback != NULL )
				{
					LOG("Calling cb %p", info->Callback);
					void	*ptr = (void *) MM_MapTemp( td->BufferPointer );
					info->Callback( info->CallbackPtr, ptr, byte_count );
					MM_FreeTemp( (tVAddr)ptr );
				}
				
				// Clean up info
				free( info );
				td->_info.ExtraInfo = NULL;
			}

			if( td->_info.bFreePointer )
				MM_DerefPhys( td->BufferPointer );			
	
			// Clean up
			td->_info.bActive = 0;
			LOG("Cleaned %p", td);
		}
	}
}

void UHCI_InterruptHandler(int IRQ, void *Ptr)
{
	tUHCI_Controller *Host = Ptr;
//	 int	frame = (_InWord(Host, FRNUM) - 1) & 0x3FF;
	Uint16	status = _InWord(Host, USBSTS);
	
	// Interrupt-on-completion
	if( status & 1 )
	{
		// TODO: Support isochronous transfers (will need updating the frame pointer)
		Semaphore_Signal(&gUHCI_InterruptSempahore, 1);
	}

	LOG("status = 0x%04x", status);
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

