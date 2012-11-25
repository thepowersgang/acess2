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
#define	MAX_CONTROLLERS	8
//#define NUM_TDs	1024
#define NUM_TDs	(PAGE_SIZE/sizeof(tUHCI_TD))
#define MAX_PACKET_SIZE	0x400
#define MAX_INTERRUPT_LOAD	1024	// Maximum bytes per frame for interrupts

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
void	*UHCI_InitInterrupt(void *Ptr, int Endpt, int bOutbound, int Period, tUSBHostCb Cb, void *CbData, void *Buf, size_t Len);
void	*UHCI_InitIsoch(void *Ptr, int Endpt, size_t MaxPacketSize);
void	*UHCI_InitControl(void *Ptr, int Endpt, size_t MaxPacketSize);
void	*UHCI_InitBulk(void *Ptr, int Endpt, size_t MaxPacketSize);
void	UHCI_RemoveEndpoint(void *Ptr, void *EndptHandle);
void	*UHCI_SendIsoch(void *Ptr, void *Dest, tUSBHostCb Cb, void *CbData, int Dir, void *Data, size_t Length, int When);
void	*UHCI_SendControl(void *Ptr, void *Dest, tUSBHostCb Cb, void *CbData,
	int isOutbound,
	const void *SetupData, size_t SetupLength,
	const void *OutData, size_t OutLength,
	void *InData, size_t InLength
	);
void	*UHCI_SendBulk(void *Ptr, void *Dest, tUSBHostCb Cb, void *CbData, int Dir, void *Data, size_t Length);

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
	.InitInterrupt = UHCI_InitInterrupt,
//	.InitIsoch     = UHCI_InitIsoch,
	.InitControl   = UHCI_InitControl,
	.InitBulk      = UHCI_InitBulk,
	.RemEndpoint   = UHCI_RemoveEndpoint,
	
//	.SendIsoch   = UHCI_SendIsoch,
	.SendControl = UHCI_SendControl,
	.SendBulk    = UHCI_SendBulk,
	.FreeOp      = NULL,
	
	.CheckPorts = UHCI_CheckPortUpdate,
//	.ClearPortFeature = NULL,
//	.GetBusState      = NULL,
//	.GetPortStatus    = NULL,
//	.SetPortFeature   = NULL
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

	if( Arguments && *Arguments && strcmp(*Arguments, "0") == 0 )
	{
		LOG("Disabled by argument");
		LEAVE('i', MODULE_ERR_NOTNEEDED);
		return MODULE_ERR_NOTNEEDED;
	}

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
		memset(gaUHCI_TDPool, 0, PAGE_SIZE);
		LOG("gaUHCI_TDPool = %p (%P)", gaUHCI_TDPool, tmp);
	}

	// Enumerate PCI Bus, getting a maximum of `MAX_CONTROLLERS` devices
	// Class:SubClass:Protocol = 0xC (Serial) : 0x3 (USB) : 0x00 (UHCI)
	while( (id = PCI_GetDeviceByClass(0x0C0300, 0xFFFFFF, id)) >= 0 && i < MAX_CONTROLLERS )
	{
		tUHCI_Controller	*cinfo = &gUHCI_Controllers[i];
		Uint32	base_addr;
		
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
	LOG("->FrameList = %p (%P)", Host->FrameList, Host->PhysFrameList);

	Host->TDQHPage = (void *) MM_AllocDMA(1, 32, &Host->PhysTDQHPage);
	if( !Host->TDQHPage ) {
		// TODO: Clean up
		Log_Warning("UHCI", "Unable to allocate QH page, aborting");
		LEAVE('i', -1);
		return -1;
	}
	LOG("->TDQHPage = %p (%P)", Host->TDQHPage, Host->PhysTDQHPage);

	// Fill frame list
	// - The numbers 0...31, but bit reversed (16 (0b1000) = 1 (0b00001)
	const int	dest_offsets[] = {
		0,16,8,24,4,20,12,28,2,18,10,26,6,22,14,30,
		1,17,9,25,5,21,13,29,3,19,11,27,7,23,15,31
		};
	// Fill all slots (but every 4th will be changed below
	for( int i = 0; i < 1024; i ++ ) {
		Uint32	addr = MM_GetPhysAddr( &Host->TDQHPage->ControlQH );
		Host->FrameList[i] = addr | 2;
	}
	for( int i = 0; i < 64; i ++ ) {
		 int	ofs = dest_offsets[ i & (32-1) ] * 2 + (i >= 32);
		Uint32	addr = Host->PhysTDQHPage + ofs * sizeof(tUHCI_QH);
		LOG("Slot %i to (%i,%i,%i,%i) ms slots",
			ofs, 0 + i*4, 256 + i*4, 512 + i*4, 768 + i*4);
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
			LOG("count=%i, dest=%p, destphys=%P", _count, dest, destphys);
			for( int i = 0; i < _count; i ++ ) {
				LOG(" %i-%i: %P==%P", _count, i, MM_GetPhysAddr(dest+i), destphys+i*sizeof(tUHCI_QH));
				dest[i].Next = destphys + (_count + i/2) * sizeof(tUHCI_QH) + 2;
				dest[i].Child = 1;
			}
			dest +=	_count; destphys += _count * sizeof(tUHCI_QH);
		}
		// Skip padding, and move to control QH
		dest->Next = MM_GetPhysAddr( &Host->TDQHPage->BulkQH ) | 2;
		dest->Child = 1;
	}

	// Set up control and bulk queues
	Host->TDQHPage->ControlQH.Next = MM_GetPhysAddr( &Host->TDQHPage->BulkQH ) | 2;
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
	LOG("Processing enabling");
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
			gaUHCI_TDPool[i].Control = TD_CTL_ACTIVE;
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
	TD->Control |= TD_CTL_IOC;
	TD->_info.QueueIndex = ((tVAddr)QH - (tVAddr)Cont->TDQHPage->InterruptQHs) / sizeof(tUHCI_QH);
	LOG("TD(%p)->QueueIndex = %i", TD, TD->_info.QueueIndex);
	// Update length
	TD->Control &= ~0x7FF;
	TD->Control |= (TD->Token >> 21) & 0x7FF;

	// Stop controller
	tPAddr	tdaddr = MM_GetPhysAddr( TD );
	ASSERT(tdaddr);
	_OutWord( Cont, USBCMD, 0x0000 );
	
	// Add
	TD->Link = 1;
	if( QH->Child & 1 ) {
		QH->Child = tdaddr;
	}
	else {
		// Depth first
		QH->_LastItem->Link = tdaddr | 4;
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
	td->Control |= TD_CTL_ACTIVE;	// Active set
	td->Control |= (3 << 27);	// 3 retries
	// High speed device (must be explicitly enabled
	if( Addr & 0x8000 )
		;
	else
		td->Control |= 1 << 26;
		
	td->Token  = ((Length - 1) & 0x7FF) << 21;
	td->Token |= (bTgl & 1) << 19;
	td->Token |= (Addr & 0xF) << 15;
	td->Token |= ((Addr/16) & 0xFF) << 8;
	td->Token |= Type;

	if(
		((tVAddr)Data & (PAGE_SIZE-1)) + Length > PAGE_SIZE
	#if PHYS_BITS > 32
		|| MM_GetPhysAddr( Data ) >> 32
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
			info->FirstPage = MM_GetPhysAddr( Data );
			info->SecondPage = MM_GetPhysAddr( (const char *)Data + Length - 1 );
		}
		else
		{
			LOG("Relocated OUT/SETUP");
			void *ptr = MM_MapTemp(td->BufferPointer);
			memcpy( ptr, Data, Length );
			MM_FreeTemp(ptr);
			td->Control |= TD_CTL_IOC;
		}
		td->_info.bFreePointer = 1;
	}
	else
	{
		td->BufferPointer = MM_GetPhysAddr( Data );
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
		td->Control |= TD_CTL_IOC;
		td->_info.ExtraInfo = info;
	}

	return td;
}

void UHCI_int_SetInterruptPoll(tUHCI_Controller *Cont, tUHCI_TD *TD, int Period)
{
	tUHCI_QH	*qh;
	const int	qh_offsets[] = {126, 124, 120, 112, 96, 64,  0};
	const int	qh_sizes[]   = {  1,   2,   4,   8, 16, 32, 64};
	
	// Bounds limit
	if( Period < 0 )	return ;
	if( Period > 256 )	Period = 256;
	if( Period == 255 )	Period = 256;

	// Get the log base2 of the period
	 int	period_slot = 0;
	while( Period >>= 1 )	period_slot ++;

	// Adjust for the 4ms minimum period
	if( period_slot < 2 )	period_slot = 0;
	else	period_slot -= 2;
	
	// _AppendTD calculates this from qh, but we use it to determine qh
	TD->_info.QueueIndex = qh_offsets[period_slot];
	// TODO: Find queue with lowest load
#if 1
	 int	min_load = 0;
	 int	min_load_slot = 0;
	for( int i = 0; i < qh_sizes[period_slot]; i ++ )
	{
		int load, index;
		index = qh_offsets[period_slot] + i;
		load = 0;
		while( index >= 0 && index < 127 )
		{
			qh = Cont->TDQHPage->InterruptQHs + index;
			load += Cont->InterruptLoad[index];
			index = ((qh->Next & ~3) - Cont->PhysTDQHPage)/sizeof(tUHCI_QH);
		}

		LOG("Slot %i (and below) load %i", qh_offsets[period_slot] + i, load);

		// i = 0 will initialise the values, otherwise update if lower
		if( i == 0 || load < min_load )
		{
			min_load = load;
			min_load_slot = i;
		}
		// - Fast return if no load
		if( load == 0 )	break;
	}
	min_load_slot += qh_offsets[period_slot];
	TD->_info.QueueIndex = min_load_slot;
	if( min_load + (TD->Control & 0x7FF) > MAX_INTERRUPT_LOAD )
	{
		Log_Warning("UHCI", "Interrupt load on %i ms is too high (slot %i load %i bytes)",
			1 << (period_slot+2), min_load_slot, min_load
			);
	}
	Cont->InterruptLoad[min_load_slot] += (TD->Control & 0x7FF);
#endif
	qh = Cont->TDQHPage->InterruptQHs + TD->_info.QueueIndex;

	LOG("period_slot = %i, QueueIndex = %i",
		period_slot, TD->_info.QueueIndex);

	// Stop any errors causing the TD to stop (NAK will error)
	// - If the device is unplugged, the removal code should remove the interrupt
	TD->Control &= ~(3 << 27);

	UHCI_int_AppendTD(Cont, qh, TD);
}

// --------------------------------------------------------------------
// API
// --------------------------------------------------------------------
void *UHCI_InitInterrupt(void *Ptr, int Endpt, int bOutbound,
	int Period, tUSBHostCb Cb, void *CbData, void *Buf, size_t Len)
{
	tUHCI_TD	*td;
	if( Period <= 0 )	return NULL;
	
	ENTER("pPtr xEndpt bbOutbound iPeriod pCb pCbData pBuf iLen",
		Ptr, Endpt, bOutbound, Period, Cb, CbData, Buf, Len);

	// TODO: Data toggle?
	td = UHCI_int_CreateTD(Ptr, Endpt, (bOutbound ? PID_OUT : PID_IN), 0, Cb, CbData, Buf, Len);
	if( !td )	return NULL;
	
	UHCI_int_SetInterruptPoll(Ptr, td, Period);

	LEAVE('p', td);
	return td;
}

void *UHCI_int_InitEndpt(tUHCI_Controller *Cont, int Type, int Endpt, size_t MaxPacketSize)
{
	if( Endpt >= 256*16 )
		return NULL;	

	if( MaxPacketSize > MAX_PACKET_SIZE) {
		Log_Warning("UHCI", "MaxPacketSize for %x greater than controller max (%i > %i)",
			Endpt, MaxPacketSize, MAX_PACKET_SIZE);
		return NULL;
	}

	if( Cont->DevInfo[Endpt / 16] == NULL ) {
		Cont->DevInfo[Endpt / 16] = calloc( 1, sizeof(*Cont->DevInfo[0]) );
	}
	tUHCI_EndpointInfo *epi = &Cont->DevInfo[Endpt/16]->EndpointInfo[Endpt%16];
	if( epi->Type ) {
		// oops, in use
		Log_Warning("UHCI", "Endpoint %x reused?", Endpt);
		return NULL;
	}

	epi->MaxPacketSize = MaxPacketSize;
	epi->Type = Type;
	epi->Tgl = 0;

	return (void*)(tVAddr)(Endpt+1);

}

void *UHCI_InitControl(void *Ptr, int Endpt, size_t MaxPacketSize)
{
	return UHCI_int_InitEndpt(Ptr, 1, Endpt, MaxPacketSize);
}

void *UHCI_InitBulk(void *Ptr, int Endpt, size_t MaxPacketSize)
{
	return UHCI_int_InitEndpt(Ptr, 2, Endpt, MaxPacketSize);
}

void UHCI_RemoveEndpoint(void *Ptr, void *Handle)
{
	tUHCI_Controller *Cont = Ptr;
	if( Handle == NULL )
		return ;
	
	if( (tVAddr)Handle <= 256*16 ) {
		 int	addr = (tVAddr)Handle;
		Cont->DevInfo[addr/16]->EndpointInfo[addr%16].Type = 0;
	}
	else {
		// TODO: Stop interrupt transaction
		Log_Error("UHCI", "TODO: Implement stopping interrupt polling");
	}
}

void *UHCI_SendControl(void *Ptr, void *Endpt, tUSBHostCb Cb, void *CbData,
	int bOutbound,	// (1) SETUP, OUT, IN vs (0) SETUP, IN, OUT
	const void *SetupData, size_t SetupLength,
	const void *OutData, size_t OutLength,
	void *InData, size_t InLength
	)
{
	ENTER("pPtr pEndpt bOutbound", Ptr, Endpt, bOutbound);
	
	tUHCI_Controller	*Cont = Ptr;
	tUHCI_QH	*qh = &Cont->TDQHPage->ControlQH;
	tUHCI_TD	*td;
	tUHCI_EndpointInfo *epi;
	 int	dest, tgl;
	size_t	mps;

	if( Endpt == NULL ) {
		Log_Error("UHCI", "Passed a NULL Endpoint handle");
		LEAVE('n');
		return NULL;
	}

	// Sanity check Endpt
	if( (tVAddr)Endpt > 0x800 ) {
		LEAVE('n');
		return NULL;
	}
	dest = (tVAddr)Endpt - 1;
	if( Cont->DevInfo[dest/16] == NULL )	LEAVE_RET('n', NULL);
	epi = &Cont->DevInfo[dest/16]->EndpointInfo[dest%16];
	if( epi->Type != 1 )	LEAVE_RET('n', NULL);
	mps = epi->MaxPacketSize;
	tgl = epi->Tgl;

	// TODO: Build up list and then append to QH in one operation

	char	*data_ptr, *status_ptr;
	size_t 	data_len,   status_len;
	Uint8	data_pid,   status_pid;
	
	if( bOutbound ) {
		data_pid   = PID_OUT; data_ptr   = (void*)OutData; data_len   = OutLength;
		status_pid = PID_IN;  status_ptr = InData;  status_len = InLength;
	}
	else {
		data_pid   = PID_IN;  data_ptr   = InData;  data_len   = InLength;
		status_pid = PID_OUT; status_ptr = (void*)OutData; status_len = OutLength;
	}

	// Sanity check data lengths
	if( SetupLength > mps )	LEAVE_RET('n', NULL);
	if( status_len > mps )	LEAVE_RET('n', NULL);

	// Create and append SETUP packet
	td = UHCI_int_CreateTD(Cont, dest, PID_SETUP, tgl, NULL, NULL, (void*)SetupData, SetupLength);
	UHCI_int_AppendTD(Cont, qh, td);
	tgl = !tgl;

	// Send data packets
	while( data_len > 0 )
	{
		size_t len = MIN(data_len, mps);
		td = UHCI_int_CreateTD(Cont, dest, data_pid, tgl, NULL, NULL, data_ptr, len);
		UHCI_int_AppendTD(Cont, qh, td);
		tgl = !tgl;
		
		data_ptr += len;
		data_len -= len;
	}
	
	// Send status
	td = UHCI_int_CreateTD(Cont, dest, status_pid, tgl, Cb, CbData, status_ptr, status_len);
	UHCI_int_AppendTD(Cont, qh, td);
	tgl = !tgl;
	
	// Update toggle value
	epi->Tgl = tgl;

	// --- HACK!!!
//	for( int i = 0; i < 1024; i ++ )
//	{
//		LOG("- FrameList[%i] = %x", i, Cont->FrameList[i]);
//	}
	// --- /HACK	

	LEAVE('p', td);	
	return td;
}

void *UHCI_SendBulk(void *Ptr, void *Endpt, tUSBHostCb Cb, void *CbData, int bOutbound, void *Data, size_t Length)
{
	tUHCI_Controller	*Cont = Ptr;
	tUHCI_QH	*qh = &Cont->TDQHPage->BulkQH;
	tUHCI_TD	*td = NULL;
	tUHCI_EndpointInfo *epi;
	 int	dest, tgl;
	size_t	mps;

	ENTER("pPtr pEndpt pCb pCbData bOutbound pData iLength", Ptr, Endpt, Cb, CbData, bOutbound, Data, Length);

	if( Endpt == NULL ) {
		Log_Error("UHCI", "_SendBulk passed a NULL endpoint handle");
		LEAVE('n');
		return NULL;
	}

	// Sanity check Endpt
	if( (tVAddr)Endpt > 256*16 ) {
		Log_Error("UHCI", "_SendBulk passed an interrupt endpoint handle");
		LEAVE('n');
		return NULL;
	}
	dest = (tVAddr)Endpt - 1;
	if( Cont->DevInfo[dest/16] == NULL ) {
		Log_Error("UHCI", "_SendBulk passed an uninitialised handle");
		LEAVE('n');
		return NULL;
	}
	epi = &Cont->DevInfo[dest/16]->EndpointInfo[dest%16];
	if( epi->Type != 2 ) {
		Log_Error("UHCI", "_SendBulk passed an invalid endpoint type (%i!=2)", epi->Type);
		LEAVE('n');
		return NULL;
	}
	tgl = epi->Tgl;
	mps = epi->MaxPacketSize;

	Uint8	pid = (bOutbound ? PID_OUT : PID_IN);

	char *pos = Data;
	while( Length > 0 )
	{
		size_t len = MIN(mps, Length);

		td = UHCI_int_CreateTD(Cont, dest, pid, tgl, Cb, (len == Length ? CbData : NULL), pos, len);
		UHCI_int_AppendTD(Cont, qh, td);
		
		pos += len;
		Length -= len;
		tgl = !tgl;
	}
	
	epi->Tgl = tgl;

	LEAVE('p', td);
	return td;
}

// ==========================
// === INTERNAL FUNCTIONS ===
// ==========================
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

	
	tPAddr	global_pool = MM_GetPhysAddr( gaUHCI_TDPool );
	
	if( PAddr < global_pool || PAddr >= global_pool + PAGE_SIZE )	return NULL;
	
	PAddr -= global_pool;
	PAddr /= sizeof(tUHCI_TD);
	return &gaUHCI_TDPool[PAddr];
}

void UHCI_int_CleanQH(tUHCI_Controller *Cont, tUHCI_QH *QH)
{
	tUHCI_TD	*td, *prev = NULL;
	Uint32	cur_td;
	 int	nCleaned = 0;

	// Disable controller
	_OutWord( Cont, USBCMD, 0x0000 );
	
	// Scan QH list
	cur_td = QH->Child;
	LOG("cur_td = 0x%08x", cur_td);
	while( !(cur_td & 1) )
	{
		td = UHCI_int_GetTDFromPhys(Cont, cur_td);
		if(!td) {
			Log_Warning("UHCI", "_int_CleanQH: QH %p contains TD %x, which was not from a pool",
				QH, cur_td);
			break ;
		}
		
		// Active? Ok.
		if( td->Control & TD_CTL_ACTIVE ) {
			LOG("%p still active", td);
			prev = td;
			cur_td = td->Link;
			continue ;
		}

		LOG("Removed %p from QH %p", td, QH);
		ASSERT(td->Link);

		if( !prev )
			QH->Child = td->Link;
		else
			prev->Link = td->Link;
		cur_td = td->Link;
		nCleaned ++;
	}

	if( nCleaned == 0 ) {
		LOG("Nothing cleaned... what the?");
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
		src = MM_MapTemp(TD->BufferPointer);
		dest = MM_MapTemp(info->FirstPage);
		// Check for a single page transfer
		if( byte_count + info->Offset <= PAGE_SIZE )
		{
			LOG("Single page copy %P to %P of %p",
				TD->BufferPointer, info->FirstPage, TD);
			memcpy(dest + info->Offset, src + src_ofs, byte_count);
		}
		else
		{
			// Multi-page
			LOG("Multi page copy %P to (%P,%P) of %p",
				TD->BufferPointer, info->FirstPage, info->SecondPage, TD);
			 int	part_len = PAGE_SIZE - info->Offset;
			memcpy(dest + info->Offset, src + src_ofs, part_len);
			MM_FreeTemp( dest );
			dest = MM_MapTemp(info->SecondPage);
			memcpy(dest, src + src_ofs + part_len, byte_count - part_len);
		}
		MM_FreeTemp( src );
		MM_FreeTemp( dest );
	}

	// Callback
	if( info->Callback != NULL )
	{
		LOG("Calling cb %p (%i bytes)", info->Callback, byte_count);
		void	*ptr = MM_MapTemp( TD->BufferPointer );
		info->Callback( info->CallbackPtr, ptr, byte_count );
		MM_FreeTemp( ptr );
	}
	
	// Clean up info
	if( TD->_info.QueueIndex > 127 )
	{
		free( info );
		TD->_info.ExtraInfo = NULL;
	}
}

void UHCI_int_InterruptThread(void *Pointer)
{
	tUHCI_Controller	*Cont = Pointer;
	Threads_SetName("UHCI Interrupt Handler");
	for( ;; )
	{
		 int	nSeen = 0;
		
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
			if( td->Control & TD_CTL_ACTIVE )	continue ;

			nSeen ++;

			// If no callback/alt buffer, mark as free and move on
			if( td->_info.ExtraInfo )
			{
				UHCI_int_HandleTDComplete(Cont, td);
			}

			// Error check
			if( td->Control & 0x00FF0000 ) {
				LOG("td->control(Status) = %s%s%s%s%s%s%s%s",
					td->Control & TD_CTL_ACTIVE     ? "Active, " : "",
					td->Control & TD_CTL_STALLED    ? "Stalled, " : "",
					td->Control & TD_CTL_DATABUFERR ? "Data Buffer Error, " : "",
					td->Control & TD_CTL_BABBLE     ? "Babble, " : "",
					td->Control & TD_CTL_NAK        ? "NAK, " : "",
					td->Control & TD_CTL_CRCERR     ? "CRC Error, " : "",
					td->Control & TD_CTL_BITSTUFF   ? "Bitstuff Error, " : "",
					td->Control & TD_CTL_RESERVED   ? "Reserved " : ""
					);
				LOG("From queue %i", td->_info.QueueIndex);
				// Clean up QH (removing all inactive entries)
				UHCI_int_CleanQH(Cont, Cont->TDQHPage->InterruptQHs + td->_info.QueueIndex);
				td->Control = 0;
			}
	
			// Handle rescheduling of interrupt TDs
			if( td->_info.QueueIndex <= 127 )
			{
				LOG("Re-schedule interrupt %p (offset %i)", td, td->_info.QueueIndex);
				// TODO: Flip toggle?
				td->Control |= TD_CTL_ACTIVE;
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

			// Clean up
			LOG("Cleaned %p (->Control = %x)", td, td->Control);
			td->_info.bActive = 0;
		}

		if( nSeen == 0 ) {
			LOG("Why did you wake me?");
		}
	}
}

void UHCI_InterruptHandler(int IRQ, void *Ptr)
{
	tUHCI_Controller *Host = Ptr;
//	 int	frame = (_InWord(Host, FRNUM) - 1) & 0x3FF;
	Uint16	status = _InWord(Host, USBSTS);
	
	LOG("%p: status = 0x%04x", Ptr, status);
	
	// Interrupt-on-completion
	if( status & 1 )
	{
		// TODO: Support isochronous transfers (will need updating the frame pointer)
		Semaphore_Signal(&gUHCI_InterruptSempahore, 1);
	}

	// USB Error Interrupt
	if( status & 2 )
	{
		Log_Notice("UHCI", "USB Error");
	}

	// Resume Detect
	// - Fired if in suspend state and a USB device sends the RESUME signal
	if( status & 4 )
	{
		Log_Notice("UHCI", "Resume Detect");
	}

	// Host System Error
	if( status & 8 )
	{
		Log_Notice("UHCI", "Host System Error");
	}

	// Host Controller Process Error
	if( status & 0x10 )
	{
		Log_Error("UHCI", "Host controller process error on controller %p", Ptr);
		// Spam Tree
		//for( int i = 0; i < 1024; i += 4 ) {
		//	LOG("%4i: %x", i, Host->FrameList[i]);
		//}
		
		tPAddr	phys = Host->TDQHPage->ControlQH.Child;
		while( !(phys & 1) && MM_GetRefCount(phys & ~15))
		{
			tUHCI_TD *td = UHCI_int_GetTDFromPhys(Host, phys);
			LOG("%08P: %08x %08x %08x", phys, td->Control, td->Token, td->BufferPointer);
			phys = td->Link;
		}
	}

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

