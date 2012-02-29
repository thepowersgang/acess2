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
#include <semaphore.h>

// === CONSTANTS ===
#define	MAX_CONTROLLERS	4
//#define NUM_TDs	1024
#define NUM_TDs	(PAGE_SIZE/sizeof(tUHCI_TD))
#define MAX_PACKET_SIZE	0x400
#define PID_IN	0x69
#define PID_OUT	0xE1
#define PID_SETUP	0x2D

// === PROTOTYPES ===
 int	UHCI_Initialise(char **Arguments);
void	UHCI_Cleanup();
 int	UHCI_int_InitHost(tUHCI_Controller *Host);
// -- List internals
tUHCI_TD	*UHCI_int_AllocateTD(tUHCI_Controller *Cont);
void	UHCI_int_AppendTD(tUHCI_Controller *Cont, tUHCI_QH *QH, tUHCI_TD *TD);
tUHCI_TD	*UHCI_int_CreateTD(tUHCI_Controller *Cont, int Addr, Uint8 Type, int bTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
// --- API
void	*UHCI_InterruptIN(void *Ptr, int Dest, int Period, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
void	*UHCI_InterruptOUT(void *Ptr, int Dest, int Period, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
void	UHCI_StopInterrupt(void *Ptr, void *Handle);
void	*UHCI_ControlSETUP(void *Ptr, int Dest, int Tgl, void *Data, size_t Length);
void	*UHCI_ControlOUT(void *Ptr, int Dest, int Tgl, tUSBHostCb Cb, void *CbData, void *Data, size_t Length);
void	*UHCI_ControlIN(void *Ptr, int Dest, int Tgl, tUSBHostCb Cb, void *CbData, void *Data, size_t Length);
void	*UHCI_BulkOUT(void *Ptr, int Dest, int bToggle, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
void	*UHCI_BulkIN(void *Ptr, int Dest, int bToggle, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);

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
tUHCI_TD	*gaUHCI_TDPool;
tUHCI_Controller	gUHCI_Controllers[MAX_CONTROLLERS];
tUSBHostDef	gUHCI_HostDef = {
	.InterruptIN   = UHCI_InterruptIN,
	.InterruptOUT  = UHCI_InterruptOUT,
	.StopInterrupt = UHCI_StopInterrupt,
	
	.ControlSETUP = UHCI_ControlSETUP,
	.ControlIN    = UHCI_ControlIN,
	.ControlOUT   = UHCI_ControlOUT,

	.BulkOUT = UHCI_BulkOUT,
	.BulkIN = UHCI_BulkIN,
	
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

	{
		tPAddr	tmp;	
		gaUHCI_TDPool = (void *) MM_AllocDMA(1, 32, &tmp);
	}

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
		ret = UHCI_int_InitHost(cinfo);
		// Detect an error
		if(ret != 0) {
			LEAVE('i', ret);
			return ret;
		}

		// Spin off interrupt handling thread
		Proc_SpawnWorker( UHCI_int_InterruptThread, cinfo );

		
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

/**
 * \brief Initialises a UHCI host controller
 * \param Host	Pointer - Host to initialise
 */
int UHCI_int_InitHost(tUHCI_Controller *Host)
{
	ENTER("pHost", Host);

	// - 1 Page, 32-bit address
	// - 1 page = 1024  4 byte entries
	Host->FrameList = (void *) MM_AllocDMA(1, 32, &Host->PhysFrameList);
	if( !Host->FrameList ) {
		Log_Warning("UHCI", "Unable to allocate frame list, aborting");
		LEAVE('i', -1);
		return -1;
	}

	Host->TDQHPage = (void *) MM_AllocDMA(1, 32, &Host->PhysTDQHPage);
	if( !Host->TDQHPage ) {
		// TODO: Clean up
		Log_Warning("UHCI", "Unable to allocate QH page, aborting");
		LEAVE('i', -1);
		return -1;
	}

	// Fill frame list
	// - The numbers 0...31, but bit reversed (16 (0b1000) = 1 (0b00001)
	const int	dest_offsets[] = {
		0,16,8,24,4,20,12,28,2,18,10,26,6,22,14,30,
		1,17,9,25,5,21,13,29,3,19,11,27,7,23,15,31
		};
	for( int i = 0; i < 1024; i ++ ) {
		Uint32	addr = MM_GetPhysAddr( (tVAddr)&Host->TDQHPage->ControlQH );
		Host->FrameList[i] = addr | 2;
	}
	for( int i = 0; i < 64; i ++ ) {
		 int	ofs = dest_offsets[ i & (32-1) ];
		Uint32	addr = Host->PhysTDQHPage + ofs * sizeof(tUHCI_QH);
		Host->FrameList[  0 + i*4] = addr | 2;
		Host->FrameList[256 + i*4] = addr | 2;
		Host->FrameList[512 + i*4] = addr | 2;
		Host->FrameList[768 + i*4] = addr | 2;
	}

	// Build up interrupt binary tree	
	{
		tUHCI_QH	*dest = Host->TDQHPage->InterruptQHs;
		Uint32	destphys = Host->PhysTDQHPage;
		
		// Set up next pointer to index to i/2 in the next step
		for( int _count = 64; _count > 1; _count /= 2 )
		{
			for( int i = 0; i < _count; i ++ ) {
				dest[i].Next = destphys + (_count + i/2) * sizeof(tUHCI_QH) + 2;
				dest[i].Child = 1;
			}
			dest +=	_count; destphys += _count * sizeof(tUHCI_QH);
		}
		// Skip padding, and move to control QH
		dest->Next = MM_GetPhysAddr( (tVAddr)&Host->TDQHPage->BulkQH ) | 2;
		dest->Child = 1;
	}

	// Set up control and bulk queues
	Host->TDQHPage->ControlQH.Next = MM_GetPhysAddr( (tVAddr)&Host->TDQHPage->BulkQH ) | 2;
	Host->TDQHPage->ControlQH.Child = 1;
	Host->TDQHPage->BulkQH.Next = 1;
	Host->TDQHPage->BulkQH.Child = 1;

	// Global reset	
	_OutWord( Host, USBCMD, 4 );
	Time_Delay(10);
	_OutWord( Host, USBCMD, 0 );
	
	// Allocate Frame List
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

// --------------------------------------------------------------------
// TDs and QH Allocation/Appending
// --------------------------------------------------------------------
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
			gaUHCI_TDPool[i]._info.QueueIndex = 128;
			Mutex_Release( &lock );
			return &gaUHCI_TDPool[i];
		}
	}
	Mutex_Release( &lock );
	return NULL;
}

void UHCI_int_AppendTD(tUHCI_Controller *Cont, tUHCI_QH *QH, tUHCI_TD *TD)
{
	static tMutex	lock;	// TODO: Should I use a shortlock (avoid being preempted)

	Mutex_Acquire(&lock);
	
	// Ensure that there is an interrupt for each used frame
	TD->Control |= (1 << 24);
	TD->_info.QueueIndex = ((tVAddr)QH - (tVAddr)Cont->TDQHPage->InterruptQHs) / sizeof(tUHCI_QH);

	// Stop controller
	_OutWord( Cont, USBCMD, 0x0000 );
	
	// Add
	TD->Link = 1;
	if( QH->Child & 1 ) {
		QH->Child = MM_GetPhysAddr( (tVAddr)TD );
	}
	else {
		// Depth first
		QH->_LastItem->Link = MM_GetPhysAddr( (tVAddr)TD ) | 4;
	}
	QH->_LastItem = TD;

	// Reenable controller
	_OutWord( Cont, USBCMD, 0x0001 );
	
	// DEBUG!
	LOG("QH(%p)->Child = %x", QH, QH->Child);
	LOG("TD(%p)->Control = %x, ->Link = %x", TD, TD->Control, TD->Link);

	Mutex_Release(&lock);
}

/**
 * \brief Send a transaction to the USB bus
 * \param Cont	Controller pointer
 * \param Addr	Function Address * 16 + Endpoint
 * \param bTgl	Data toggle value
 */
tUHCI_TD *UHCI_int_CreateTD(
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
		Log_Error("UHCI", "No avaliable TDs, transaction dropped");
		return NULL;
	}

	LOG("TD %p %i bytes, Type %x to 0x%x",
		td, Length, Type, Addr);

	td->Control = (Length - 1) & 0x7FF;
	td->Control |= (1 << 23);	// Active set
	td->Control |= (3 << 27);	// 3 retries
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
		info->Callback = Cb;
		info->CallbackPtr = CbData;
	}
	
	if( info ) {
		LOG("info = %p", info);
		td->Control |= (1 << 24);
		td->_info.ExtraInfo = info;
	}

	return td;
}

void UHCI_int_SetInterruptPoll(tUHCI_Controller *Cont, tUHCI_TD *TD, int Period)
{
	tUHCI_QH	*qh;
	const int	qh_offsets[] = { 0, 64, 96, 112, 120, 124, 126};
//	const int	qh_sizes[]   = {64, 32, 16,   8,   4,   2,   1};
	
	// Bounds limit
	if( Period < 0 )	return ;
	if( Period > 256 )	Period = 256;

	// Get the log base2 of the period
	 int	period_slot = 0;
	while( Period >>= 1 )	period_slot ++;

	// Adjust for the 4ms minimum period
	if( period_slot < 2 )	period_slot = 0;
	else	period_slot -= 2;
	
	TD->_info.QueueIndex = qh_offsets[period_slot];	// Actually re-filled in _AppendTD, but meh
	qh = Cont->TDQHPage->InterruptQHs + TD->_info.QueueIndex;
	// TODO: Find queue with lowest load

	LOG("period_slot = %i, QueueIndex = %i",
		period_slot, TD->_info.QueueIndex);

	// Stop any errors causing the TD to stop (NAK will error)
	// - If the device goes away, the interrupt should be stopped anyway
	TD->Control &= ~(3 << 27);

	UHCI_int_AppendTD(Cont, qh, TD);
}

void *UHCI_InterruptIN(void *Ptr, int Dest, int Period, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	tUHCI_TD	*td;

	if( Period < 0 )	return NULL;

	ENTER("pPtr xDest iPeriod pCb pCbData pBuf, iLength",
		Ptr, Dest, Period, Cb, CbData, Buf, Length);

	// TODO: Data toggle?
	td = UHCI_int_CreateTD(Ptr, Dest, PID_IN, 0, Cb, CbData, Buf, Length);
	if( !td )	return NULL;
	
	UHCI_int_SetInterruptPoll(Ptr, td, Period);
	
	LEAVE('p', td);	
	return td;
}
// TODO: Does interrupt OUT make sense?
void *UHCI_InterruptOUT(void *Ptr, int Dest, int Period, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	tUHCI_TD	*td;

	if( Period < 0 )	return NULL;

	ENTER("pPtr xDest iPeriod pCb pCbData pBuf, iLength",
		Ptr, Dest, Period, Cb, CbData, Buf, Length);

	// TODO: Data toggle?
	td = UHCI_int_CreateTD(Ptr, Dest, PID_OUT, 0, Cb, CbData, Buf, Length);
	if( !td )	return NULL;
	
	UHCI_int_SetInterruptPoll(Ptr, td, Period);

	LEAVE('p', td);	
	return td;
}

void UHCI_StopInterrupt(void *Ptr, void *Handle)
{
	// TODO: Stop interrupt transaction
	Log_Error("UHCI", "TODO: Implement UHCI_StopInterrupt");
}

void *UHCI_ControlSETUP(void *Ptr, int Dest, int Tgl, void *Data, size_t Length)
{
	tUHCI_Controller	*Cont = Ptr;
	tUHCI_QH	*qh = &Cont->TDQHPage->ControlQH;
	tUHCI_TD	*td;

	ENTER("pPtr xDest iTgl pData iLength", Ptr, Dest, Tgl, Data, Length);
	
	td = UHCI_int_CreateTD(Cont, Dest, PID_SETUP, Tgl, NULL, NULL, Data, Length);
	UHCI_int_AppendTD(Cont, qh, td);

	LEAVE('p', td);	

	return td;
}
void *UHCI_ControlOUT(void *Ptr, int Dest, int Tgl, tUSBHostCb Cb, void *CbData, void *Data, size_t Length)
{
	tUHCI_Controller	*Cont = Ptr;
	tUHCI_QH	*qh = &Cont->TDQHPage->ControlQH;
	tUHCI_TD	*td;

	ENTER("pPtr xDest iTgl pCb pCbData pData iLength", Ptr, Dest, Tgl, Cb, CbData, Data, Length);

	td = UHCI_int_CreateTD(Cont, Dest, PID_OUT, Tgl, Cb, CbData, Data, Length);
	UHCI_int_AppendTD(Cont, qh, td);

	LEAVE('p', td);
	return td;
}
void *UHCI_ControlIN(void *Ptr, int Dest, int Tgl, tUSBHostCb Cb, void *CbData, void *Data, size_t Length)
{
	tUHCI_Controller	*Cont = Ptr;
	tUHCI_QH	*qh = &Cont->TDQHPage->ControlQH;
	tUHCI_TD	*td;

	ENTER("pPtr xDest iTgl pCb pCbData pData iLength", Ptr, Dest, Tgl, Cb, CbData, Data, Length);
	
	td = UHCI_int_CreateTD(Cont, Dest, PID_IN, !!Tgl, Cb, CbData, Data, Length);
	UHCI_int_AppendTD(Cont, qh, td);

	LEAVE('p', td);
	return td;
}

void *UHCI_BulkOUT(void *Ptr, int Dest, int bToggle, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	tUHCI_Controller	*Cont = Ptr;
	tUHCI_QH	*qh = &Cont->TDQHPage->BulkQH;
	tUHCI_TD	*td;
	char	*src = Buf;

	ENTER("pPtr xDest ibToggle pCb pCbData pData iLength", Ptr, Dest, bToggle, Cb, CbData, Buf, Length);

	while( Length > MAX_PACKET_SIZE )
	{
		LOG("MaxPacket (rem = %i)", Length);
		td = UHCI_int_CreateTD(Cont, Dest, PID_OUT, bToggle, NULL, NULL, src, MAX_PACKET_SIZE);
		UHCI_int_AppendTD(Cont, qh, td);
		
		bToggle = !bToggle;
		Length -= MAX_PACKET_SIZE;
		src += MAX_PACKET_SIZE;
	}

	LOG("Final");
	td = UHCI_int_CreateTD(Cont, Dest, PID_OUT, bToggle, NULL, NULL, src, Length);
	UHCI_int_AppendTD(Cont, qh, td);

	LEAVE('p', td);
	return td;
}
void *UHCI_BulkIN(void *Ptr, int Dest, int bToggle, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	tUHCI_Controller	*Cont = Ptr;
	tUHCI_QH	*qh = &Cont->TDQHPage->BulkQH;
	tUHCI_TD	*td;
	char	*dst = Buf;

	ENTER("pPtr xDest ibToggle pCb pCbData pData iLength", Ptr, Dest, bToggle, Cb, CbData, Buf, Length);
	while( Length > MAX_PACKET_SIZE )
	{
		LOG("MaxPacket (rem = %i)", Length);
		td = UHCI_int_CreateTD(Cont, Dest, PID_IN, bToggle, NULL, NULL, dst, MAX_PACKET_SIZE);
		UHCI_int_AppendTD(Cont, qh, td);
		
		bToggle = !bToggle;
		Length -= MAX_PACKET_SIZE;
		dst += MAX_PACKET_SIZE;
	}

	LOG("Final");
	td = UHCI_int_CreateTD(Cont, Dest, PID_IN, bToggle, NULL, NULL, dst, Length);
	UHCI_int_AppendTD(Cont, qh, td);

	LEAVE('p', td);
	return td;
}

// === INTERNAL FUNCTIONS ===
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

tUHCI_TD *UHCI_int_GetTDFromPhys(tUHCI_Controller *Controller, Uint32 PAddr)
{
	if( PAddr >= Controller->PhysTDQHPage && PAddr < Controller->PhysTDQHPage + PAGE_SIZE )
	{
		PAddr -= Controller->PhysTDQHPage;
		PAddr -= (128+2)*sizeof(tUHCI_QH);
		if( PAddr > PAGE_SIZE )	return NULL;	// Wrapping will bring above page size
		PAddr /= sizeof(tUHCI_TD);
		return &Controller->TDQHPage->LocalTDPool[PAddr];
	}

	
	tPAddr	global_pool = MM_GetPhysAddr( (tVAddr)gaUHCI_TDPool );
	
	if( PAddr < global_pool || PAddr >= global_pool + PAGE_SIZE )	return NULL;
	
	PAddr -= global_pool;
	PAddr /= sizeof(tUHCI_TD);
	return &gaUHCI_TDPool[PAddr];
}

void UHCI_int_CleanQH(tUHCI_Controller *Cont, tUHCI_QH *QH)
{
	tUHCI_TD	*td, *prev = NULL;
	Uint32	cur_td;

	// Disable controller
	_OutWord( Cont, USBCMD, 0x0000 );
	
	// Scan QH list
	cur_td = QH->Child;
	while( !(cur_td & 1) )
	{
		td = UHCI_int_GetTDFromPhys(Cont, cur_td);
		if(!td) {
			Log_Warning("UHCI", "_int_CleanQH: QH %p contains TD %x, which was not from a pool",
				QH, cur_td);
			break ;
		}
		
		// Active? Ok.
		if( td->Control & (1 << 23) ) {
			prev = td;
			continue ;
		}

		LOG("Removed %p from QH %p", td, QH);		

		if( !prev )
			QH->Child = td->Link;
		else
			prev->Link = td->Link;
		cur_td = td->Link;
	}

	// re-enable controller
	_OutWord( Cont, USBCMD, 0x0001 );	
}

void UHCI_int_HandleTDComplete(tUHCI_Controller *Cont, tUHCI_TD *TD)
{
	 int	byte_count = (TD->Control & 0x7FF)+1;
	tUHCI_ExtraTDInfo	*info = TD->_info.ExtraInfo;

	// Handle non page-aligned destination (or with a > 32-bit paddr)
	// TODO: Needs fixing for alignment issues
	if(info->FirstPage)
	{
		char	*src, *dest;
		 int	src_ofs = TD->BufferPointer & (PAGE_SIZE-1);
		src = (void *) MM_MapTemp(TD->BufferPointer);
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

	// Callback
	if( info->Callback != NULL )
	{
		LOG("Calling cb %p", info->Callback);
		void	*ptr = (void *) MM_MapTemp( TD->BufferPointer );
		info->Callback( info->CallbackPtr, ptr, byte_count );
		MM_FreeTemp( (tVAddr)ptr );
	}
	
	// Clean up info
	free( info );
	TD->_info.ExtraInfo = NULL;
}

void UHCI_int_InterruptThread(void *Pointer)
{
	tUHCI_Controller	*Cont = Pointer;
	Threads_SetName("UHCI Interrupt Handler");
	for( ;; )
	{
		LOG("zzzzz....");
		// 0 = Take all
		Semaphore_Wait(&gUHCI_InterruptSempahore, 0);
		LOG("Huh?");
	
		for( int i = 0; i < NUM_TDs; i ++ )
		{
			tUHCI_TD	*td;
			
			td = &gaUHCI_TDPool[i];

			// Skip completely inactive TDs
			if( td->_info.bActive == 0 )	continue ;
			// Skip ones that are still in use
			if( td->Control & (1 << 23) )	continue ;

			// If no callback/alt buffer, mark as free and move on
			if( td->_info.ExtraInfo )
			{
				UHCI_int_HandleTDComplete(Cont, td);
			}

			// Handle rescheduling of interrupt TDs
			if( td->_info.QueueIndex <= 127 )
			{
				LOG("Re-schedule interrupt %p (offset %i)", td, td->_info.QueueIndex);
				// TODO: Flip toggle?
				td->Control |= (1 << 23);
				// Add back into controller's interrupt list
				UHCI_int_AppendTD(
					Cont,
					Cont->TDQHPage->InterruptQHs + td->_info.QueueIndex,
					td
					);
				continue ;
			}

			// Clean up
			if( td->_info.bFreePointer )
				MM_DerefPhys( td->BufferPointer );

			// Error check
			if( td->Control & 0x00FF0000 ) {
				LOG("td->control(Status) = %s%s%s%s%s%s%s%s",
					td->Control & (1 << 23) ? "Active " : "",
					td->Control & (1 << 22) ? "Stalled " : "",
					td->Control & (1 << 21) ? "Data Buffer Error " : "",
					td->Control & (1 << 20) ? "Babble " : "",
					td->Control & (1 << 19) ? "NAK " : "",
					td->Control & (1 << 18) ? "CRC Error, " : "",
					td->Control & (1 << 17) ? "Bitstuff Error, " : "",
					td->Control & (1 << 16) ? "Reserved " : ""
					);
				// Clean up QH (removing all inactive entries)
				UHCI_int_CleanQH(Cont, Cont->TDQHPage->InterruptQHs + td->_info.QueueIndex);
			}
	
			// Clean up
			LOG("Cleaned %p (->Control = %x)", td, td->Control);
			td->_info.bActive = 0;
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

