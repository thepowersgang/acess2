/*
 * Acess2 EHCI Driver
 * - By John Hodge (thePowersGang)
 * 
 * ehci.c
 * - Driver Core
 */
#define DEBUG	1
#define VERSION	VER2(0,1)
#include <acess.h>
#include <modules.h>
#include <usb_host.h>
#include "ehci.h"
#include <drv_pci.h>
#include <limits.h>
#include <events.h>
#include <timers.h>

// === CONSTANTS ===
#define EHCI_MAX_CONTROLLERS	4
#define EHCI_THREADEVENT_IOC	THREAD_EVENT_USER1
#define EHCI_THREADEVENT_PORTSC	THREAD_EVENT_USER2

// === PROTOTYPES ===
 int	EHCI_Initialise(char **Arguments);
 int	EHCI_Cleanup(void);
 int	EHCI_InitController(tPAddr BaseAddress, Uint8 InterruptNum);
void 	EHCI_InterruptHandler(int IRQ, void *Ptr);
// -- API ---
void	*EHCI_InitInterrupt(void *Ptr, int Endpoint, int bInput, int Period, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
void	*EHCI_InitIsoch  (void *Ptr, int Endpoint, size_t MaxPacketSize);
void	*EHCI_InitControl(void *Ptr, int Endpoint, size_t MaxPacketSize);
void	*EHCI_InitBulk   (void *Ptr, int Endpoint, size_t MaxPacketSize);
void	EHCI_RemEndpoint(void *Ptr, void *Handle);
void	*EHCI_SendControl(void *Ptr, void *Dest, tUSBHostCb Cb, void *CbData,
	int isOutbound,
	const void *SetupData, size_t SetupLength,
	const void *OutData, size_t OutLength,
	void *InData, size_t InLength
	);
void	*EHCI_SendBulk(void *Ptr, void *Dest, tUSBHostCb Cb, void *CbData, int Dir, void *Data, size_t Length);
void	EHCI_FreeOp(void *Ptr, void *Handle);
Uint32	EHCI_int_RootHub_FeatToMask(int Feat);
void	EHCI_RootHub_SetPortFeature(void *Ptr, int Port, int Feat);
void	EHCI_RootHub_ClearPortFeature(void *Ptr, int Port, int Feat);
 int	EHCI_RootHub_GetPortStatus(void *Ptr, int Port, int Flag);
// --- Internals ---
tEHCI_qTD	*EHCI_int_AllocateTD(tEHCI_Controller *Cont, int PID, void *Data, size_t Length, tUSBHostCb Cb, void *CbData);
void	EHCI_int_DeallocateTD(tEHCI_Controller *Cont, tEHCI_qTD *TD);
void	EHCI_int_AppendTD(tEHCI_Controller *Cont, tEHCI_QH *QH, tEHCI_qTD *TD);
tEHCI_QH	*EHCI_int_AllocateQH(tEHCI_Controller *Cont, int Endpoint, size_t MaxPacketSize);
void	EHCI_int_DeallocateQH(tEHCI_Controller *Cont, tEHCI_QH *QH);
void	EHCI_int_InterruptThread(void *ControllerPtr);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, USB_EHCI, EHCI_Initialise, NULL, "USB_Core", NULL);
tEHCI_Controller	gaEHCI_Controllers[EHCI_MAX_CONTROLLERS];
tUSBHostDef	gEHCI_HostDef = {
	.InitInterrupt = EHCI_InitInterrupt,
	.InitIsoch     = EHCI_InitIsoch,
	.InitControl   = EHCI_InitControl,
	.InitBulk      = EHCI_InitBulk,
	.RemEndpoint   = EHCI_RemEndpoint,
	.SendIsoch   = NULL,
	.SendControl = EHCI_SendControl,
	.SendBulk    = EHCI_SendBulk,
	.FreeOp      = EHCI_FreeOp,
	
	.CheckPorts = NULL,	// No need
	.SetPortFeature   = EHCI_RootHub_SetPortFeature,
	.ClearPortFeature = EHCI_RootHub_ClearPortFeature,
	.GetPortStatus    = EHCI_RootHub_GetPortStatus,
	};

// === CODE ===
int EHCI_Initialise(char **Arguments)
{
	for( int id = -1; (id = PCI_GetDeviceByClass(0x0C0320, 0xFFFFFF, id)) >= 0;  )
	{
		Uint32	addr = PCI_GetBAR(id, 0);
		if( addr == 0 ) {
			// Oops, PCI BIOS emulation time
		}
		Uint8	irq = PCI_GetIRQ(id);
		if( irq == 0 ) {
			// TODO: The same
		}

		Log_Log("ECHI", "Controller at PCI %i 0x%x IRQ 0x%x",
			id, addr, irq);

		if( EHCI_InitController(addr, irq) )
		{
			// TODO: Detect other forms of failure than "out of slots"
			break ;
		}

		// TODO: Register with the USB stack
	}
	return 0;
}

int EHCI_Cleanup(void)
{
	return 0;
}

// --- Driver Init ---
int EHCI_InitController(tPAddr BaseAddress, Uint8 InterruptNum)
{
	tEHCI_Controller	*cont = NULL;

	for( int i = 0; i < EHCI_MAX_CONTROLLERS; i ++ )
	{
		if( gaEHCI_Controllers[i].PhysBase == 0 ) {
			cont = &gaEHCI_Controllers[i];
			cont->PhysBase = BaseAddress;
			break;
		}
	}
	if(!cont) {
		Log_Notice("EHCI", "Too many controllers (EHCI_MAX_CONTROLLERS=%i)",
			EHCI_MAX_CONTROLLERS);
		return 1;
	}

	// - Nuke a couple of fields so error handling code doesn't derp
	cont->CapRegs = NULL;
	cont->PeriodicQueue = NULL;
	cont->TDPool = NULL;

	// -- Build up structure --
	cont->CapRegs = (void*)MM_MapHWPages(BaseAddress, 1);
	if( !cont->CapRegs ) {
		Log_Warning("EHCI", "Can't map 1 page at %P into kernel space", BaseAddress);
		goto _error;
	}
	// TODO: Error check
	if( (cont->CapRegs->CapLength & 3) ) {
		Log_Warning("EHCI", "Controller at %P non-aligned op regs", BaseAddress);
		goto _error;
	}
	cont->OpRegs = (void*)( (Uint32*)cont->CapRegs + cont->CapRegs->CapLength / 4 );
	// - Allocate periodic queue
	tPAddr	unused;
	cont->PeriodicQueue = (void*)MM_AllocDMA(1, 32, &unused);
	if( !cont->PeriodicQueue ) {
		Log_Warning("ECHI", "Can't allocate 1 32-bit page for periodic queue");
		goto _error;
	}
	for( int i = 0; i < 1024; i ++ )
		cont->PeriodicQueue[i] = 1;
	// TODO: Error check
	//  > Populate queue

	// - Allocate TD pool
	cont->TDPool = (void*)MM_AllocDMA(1, 32, &unused);
	if( !cont->TDPool ) {
		Log_Warning("ECHI", "Can't allocate 1 32-bit page for qTD pool");
		goto _error;
	}
	for( int i = 0; i < TD_POOL_SIZE; i ++ ) {
		cont->TDPool[i].Token = 3 << 8;
	}

	// Get port count
	cont->nPorts = cont->CapRegs->HCSParams & 0xF;

	// -- Bind IRQ --
	IRQ_AddHandler(InterruptNum, EHCI_InterruptHandler, cont);
	cont->InterruptThread = Proc_SpawnWorker(EHCI_int_InterruptThread, cont);
	if( !cont->InterruptThread ) {
		Log_Warning("EHCI", "Can't spawn interrupt worker thread");
		goto _error;
	}
	LOG("cont->InterruptThread = %p", cont->InterruptThread);

	// -- Initialisation procedure (from ehci-r10) --
	// - Reset controller
	cont->OpRegs->USBCmd = USBCMD_HCReset;
	// - Set CTRLDSSEGMENT (TODO: 64-bit support)
	// - Set USBINTR
	cont->OpRegs->USBIntr = USBINTR_IOC|USBINTR_PortChange|USBINTR_FrameRollover;
	// - Set PERIODICLIST BASE
	cont->OpRegs->PeridocListBase = MM_GetPhysAddr( cont->PeriodicQueue );
	// - Enable controller
	cont->OpRegs->USBCmd = (0x40 << 16) | USBCMD_PeriodicEnable | USBCMD_Run;
	// - Route all ports
	cont->OpRegs->ConfigFlag = 1;

	cont->DeadTD = EHCI_int_AllocateTD(cont, 0, NULL, 0, NULL, NULL);
	cont->DeadTD->Link = 1;
	cont->DeadTD->Link2 = 1;
	cont->DeadTD->Token = 0;

	// -- Register with USB Core --
	cont->RootHub = USB_RegisterHost(&gEHCI_HostDef, cont, cont->nPorts);

	return 0;
_error:
	cont->PhysBase = 0;
	if( cont->CapRegs )
		MM_Deallocate( (tVAddr)cont->CapRegs );
	if( cont->PeriodicQueue )
		MM_Deallocate( (tVAddr)cont->PeriodicQueue );
	if( cont->TDPool )
		MM_Deallocate( (tVAddr)cont->TDPool );
	return 2;
}

void EHCI_InterruptHandler(int IRQ, void *Ptr)
{
	tEHCI_Controller *Cont = Ptr;
	Uint32	sts = Cont->OpRegs->USBSts;
	
	// Clear interrupts
	Cont->OpRegs->USBSts = sts;	

	if( sts & 0xFFFF0FC0 ) {
		LOG("Oops, reserved bits set (%08x), funny hardware?", sts);
		sts &= ~0xFFFF0FFC0;
	}

	// Unmask read-only bits
	sts &= ~(0xF000);

	if( sts & USBINTR_IOC ) {
		// IOC
		Threads_PostEvent(Cont->InterruptThread, EHCI_THREADEVENT_IOC);
		sts &= ~USBINTR_IOC;
	}

	if( sts & USBINTR_PortChange ) {
		// Port change, determine what port and poke helper thread
		LOG("Port status change");
		Threads_PostEvent(Cont->InterruptThread, EHCI_THREADEVENT_PORTSC);
		sts &= ~USBINTR_PortChange;
	}
	
	if( sts & USBINTR_FrameRollover ) {
		// Frame rollover, used to aid timing (trigger per-second operations)
		LOG("Frame rollover");
		sts &= ~USBINTR_FrameRollover;
	}

	if( sts ) {
		// Unhandled interupt bits
		// TODO: Warn
		LOG("WARN - Bitmask %x unhandled", sts);
	}


}

// --------------------------------------------------------------------
// USB API
// --------------------------------------------------------------------
void *EHCI_InitInterrupt(void *Ptr, int Endpoint, int bOutbound, int Period,
	tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	tEHCI_Controller	*Cont = Ptr;
	 int	pow2period, period_pow;
	
	if( Endpoint >= 256*16 )
		return NULL;
	if( Period <= 0 )
		return NULL;
	if( Period > 256 )
		Period = 256;

	// Round the period to the closest power of two
	pow2period = 1;
	period_pow = 0;
	// - Find the first power above the period
	while( pow2period < Period )
	{
		pow2period *= 2;
		period_pow ++;
	}
	// - Check which is closest
	if( Period - pow2period / 2 > pow2period - Period )
		Period = pow2period;
	else {
		Period = pow2period/2;
		period_pow --;
	}
	
	// Allocate a QH
	tEHCI_QH *qh = EHCI_int_AllocateQH(Cont, Endpoint, Length);
	qh->Impl.IntPeriodPow = period_pow;

	Mutex_Acquire(&Cont->PeriodicListLock);

	// Choose an interrupt slot to use	
	int minslot = 0, minslot_load = INT_MAX;
	for( int slot = 0; slot < Period; slot ++ )
	{
		 int	load = 0;
		for( int i = 0; i < PERIODIC_SIZE; i += Period )
			load += Cont->InterruptLoad[i+slot];
		if( load == 0 )	break;
		if( load < minslot_load ) {
			minslot = slot;
			minslot_load = load;
		}
	}
	// Increase loading on the selected slot
	for( int i = minslot; i < PERIODIC_SIZE; i += Period )
		Cont->InterruptLoad[i] += Length;
	qh->Impl.IntOfs = minslot;

	// Allocate TD for the data
	tEHCI_qTD *td = EHCI_int_AllocateTD(Cont, (bOutbound ? PID_OUT : PID_IN), Buf, Length, Cb, CbData);
	EHCI_int_AppendTD(Cont, qh, td);

	// Insert into the periodic list
	for( int i = 0; i < PERIODIC_SIZE; i += Period )
	{
		// Walk list until
		// - the end is reached
		// - this QH is found
		// - A QH with a lower period is encountered
		tEHCI_QH	*pqh = NULL;
		tEHCI_QH	*nqh;
		for( nqh = Cont->PeriodicQueueV[i]; nqh; pqh = nqh, nqh = nqh->Impl.Next )
		{
			if( nqh == qh )
				break;
			if( nqh->Impl.IntPeriodPow < period_pow )
				break;
		}

		// Somehow, we've already been added to this queue.
		if( nqh && nqh == qh )
			continue ;

		if( qh->Impl.Next && qh->Impl.Next != nqh ) {
			Log_Warning("EHCI", "Suspected bookkeeping error on %p - int list %i+%i overlap",
				Cont, period_pow, minslot);
			break;
		}

		if( nqh ) {
			qh->Impl.Next = nqh;
			qh->HLink = MM_GetPhysAddr(nqh) | 2;
		}
		else {
			qh->Impl.Next = NULL;
			qh->HLink = 2|1;	// QH, Terminate
		}

		if( pqh ) {
			pqh->Impl.Next = qh;
			pqh->HLink = MM_GetPhysAddr(qh) | 2;
		}
		else {
			Cont->PeriodicQueueV[i] = qh;
			Cont->PeriodicQueue[i] = MM_GetPhysAddr(qh) | 2;
		}
	}
	Mutex_Release(&Cont->PeriodicListLock);

	return qh;
}

void *EHCI_InitIsoch(void *Ptr, int Endpoint, size_t MaxPacketSize)
{
	return (void*)(tVAddr)(Endpoint + 1);
}
void *EHCI_InitControl(void *Ptr, int Endpoint, size_t MaxPacketSize)
{
	tEHCI_Controller *Cont = Ptr;
	
	// Allocate a QH
	tEHCI_QH *qh = EHCI_int_AllocateQH(Cont, Endpoint, MaxPacketSize);
	qh->CurrentTD = MM_GetPhysAddr(Cont->DeadTD);

	// Append to async list	
	if( Cont->LastAsyncHead ) {
		Cont->LastAsyncHead->HLink = MM_GetPhysAddr(qh)|2;
		Cont->LastAsyncHead->Impl.Next = qh;
		LOG("- Placed after %p", Cont->LastAsyncHead);
	}
	else {
		Cont->OpRegs->AsyncListAddr = MM_GetPhysAddr(qh)|2;
	}
	qh->HLink = Cont->OpRegs->AsyncListAddr;
	Cont->OpRegs->USBCmd |= USBCMD_AsyncEnable;
	Cont->LastAsyncHead = qh;

	LOG("Created %p for %p Ep 0x%x - %i bytes MPS", qh, Ptr, Endpoint, MaxPacketSize);

	return qh;
}
void *EHCI_InitBulk(void *Ptr, int Endpoint, size_t MaxPacketSize)
{
	return EHCI_InitControl(Ptr, Endpoint, MaxPacketSize);
}
void EHCI_RemEndpoint(void *Ptr, void *Handle)
{
	if( Handle == NULL )
		return ;
	else if( (tVAddr)Handle <= 256*16 )
		return ;	// Isoch
	else {
		tEHCI_QH	*qh = Ptr;

		// Remove QH from list
		// - If it's a polling endpoint, need to remove from all periodic lists
		if( qh->Impl.IntPeriodPow != 0xFF) {
			// Poll
		}
		else {
			// GP
		}
		
		// Deallocate QH
		EHCI_int_DeallocateQH(Ptr, Handle);
	}
}

void *EHCI_SendControl(void *Ptr, void *Dest, tUSBHostCb Cb, void *CbData,
	int isOutbound,
	const void *SetupData, size_t SetupLength,
	const void *OutData, size_t OutLength,
	void *InData, size_t InLength
	)
{
	tEHCI_Controller *Cont = Ptr;
	tEHCI_qTD	*td_setup, *td_data, *td_status;

	// Sanity checks
	if( (tVAddr)Dest <= 256*16 )
		return NULL;

	// Check size of SETUP and status
	
	// Allocate TDs
	td_setup = EHCI_int_AllocateTD(Cont, PID_SETUP, (void*)SetupData, SetupLength, NULL, NULL);
	if( isOutbound )
	{
		td_data = OutData ? EHCI_int_AllocateTD(Cont, PID_OUT, (void*)OutData, OutLength, NULL, NULL) : NULL;
		td_status = EHCI_int_AllocateTD(Cont, PID_IN, InData, InLength, Cb, CbData);
	}
	else
	{
		td_data = InData ? EHCI_int_AllocateTD(Cont, PID_IN, InData, InLength, NULL, NULL) : NULL;
		td_status = EHCI_int_AllocateTD(Cont, PID_OUT, (void*)OutData, OutLength, Cb, CbData);
		td_status->Token |= (1 << 15);	// IOC
	}

	// Append TDs
	if( td_data ) {
		td_setup->Link = MM_GetPhysAddr(td_data);
		td_data->Link = MM_GetPhysAddr(td_status) | 1;
		td_data->Token |= (1 << 8);	// Active
	}
	else {
		td_setup->Link = MM_GetPhysAddr(td_status) | 1;
	}
	td_setup->Token |= (1 << 8);	// Active
	td_status->Token |= (1 << 8);
	EHCI_int_AppendTD(Cont, Dest, td_setup);

	return td_status;
}

void *EHCI_SendBulk(void *Ptr, void *Dest, tUSBHostCb Cb, void *CbData, int Dir, void *Data, size_t Length)
{
	tEHCI_Controller	*Cont = Ptr;
	
	// Sanity check the pointer
	// - Can't be NULL or an isoch
	if( (tVAddr)Dest <= 256*16 )
		return NULL;
	
	// Allocate single TD
	tEHCI_qTD	*td = EHCI_int_AllocateTD(Cont, (Dir ? PID_OUT : PID_IN), Data, Length, Cb, CbData);
	EHCI_int_AppendTD(Cont, Dest, td);	

	return td;
}

void EHCI_FreeOp(void *Ptr, void *Handle)
{
	tEHCI_Controller	*Cont = Ptr;

	EHCI_int_DeallocateTD(Cont, Handle);
}

Uint32 EHCI_int_RootHub_FeatToMask(int Feat)
{
	switch(Feat)
	{
	case PORT_RESET:	return PORTSC_PortReset;
	case PORT_ENABLE:	return PORTSC_PortEnabled;
	default:
		Log_Warning("EHCI", "Unknown root hub port feature %i", Feat);
		return 0;
	}
}

void EHCI_RootHub_SetPortFeature(void *Ptr, int Port, int Feat)
{
	tEHCI_Controller	*Cont = Ptr;
	if(Port >= Cont->nPorts)	return;

	Cont->OpRegs->PortSC[Port] |= EHCI_int_RootHub_FeatToMask(Feat);
}

void EHCI_RootHub_ClearPortFeature(void *Ptr, int Port, int Feat)
{
	tEHCI_Controller	*Cont = Ptr;
	if(Port >= Cont->nPorts)	return;

	Cont->OpRegs->PortSC[Port] &= ~EHCI_int_RootHub_FeatToMask(Feat);
}

int EHCI_RootHub_GetPortStatus(void *Ptr, int Port, int Flag)
{
	tEHCI_Controller	*Cont = Ptr;
	if(Port >= Cont->nPorts)	return 0;

	return !!(Cont->OpRegs->PortSC[Port] & EHCI_int_RootHub_FeatToMask(Flag));
}

// --------------------------------------------------------------------
// Internals
// --------------------------------------------------------------------
tEHCI_qTD *EHCI_int_GetTDFromPhys(tEHCI_Controller *Cont, Uint32 Addr)
{
	if( Addr == 0 )	return NULL;
	LOG("%p + (%x - %x)", Cont->TDPool, Addr, MM_GetPhysAddr(Cont->TDPool));
	return Cont->TDPool + (Addr - MM_GetPhysAddr(Cont->TDPool))/sizeof(tEHCI_qTD);
}

tEHCI_qTD *EHCI_int_AllocateTD(tEHCI_Controller *Cont, int PID, void *Data, size_t Length, tUSBHostCb Cb, void *CbData)
{
//	Semaphore_Wait(&Cont->TDSemaphore, 1);
	Mutex_Acquire(&Cont->TDPoolMutex);
	for( int i = 0; i < TD_POOL_SIZE; i ++ )
	{
		if( ((Cont->TDPool[i].Token >> 8) & 3) != 3 )
			continue ;
		Cont->TDPool[i].Token = (PID << 8) | (Length << 16);
		// NOTE: Assumes that `Length` is <= PAGE_SIZE
		Cont->TDPool[i].Pages[0] = MM_GetPhysAddr(Data);
		if( (Cont->TDPool[i].Pages[0] & (PAGE_SIZE-1)) + Length - 1 > PAGE_SIZE )
			Cont->TDPool[i].Pages[1] = MM_GetPhysAddr((char*)Data + Length - 1) & ~(PAGE_SIZE-1);
		Mutex_Release(&Cont->TDPoolMutex);
		LOG("Allocated %p for PID %i on %p", &Cont->TDPool[i], PID, Cont);
		return &Cont->TDPool[i];
	}

	Mutex_Release(&Cont->TDPoolMutex);
	return NULL;
}

void EHCI_int_DeallocateTD(tEHCI_Controller *Cont, tEHCI_qTD *TD)
{
	UNIMPLEMENTED();
}

void EHCI_int_AppendTD(tEHCI_Controller *Cont, tEHCI_QH *QH, tEHCI_qTD *TD)
{
	tEHCI_qTD	*ptd = NULL;
	Uint32	link = QH->CurrentTD;

	// TODO: Need locking and validation here
	while( link && !(link & 1) )
	{
		ptd = EHCI_int_GetTDFromPhys(Cont, link);
		link = ptd->Link;
	}
	// TODO: Figure out how to follow this properly
	if( !ptd ) {
		QH->CurrentTD = MM_GetPhysAddr(TD);
		LOG("Appended %p to beginning of %p", TD, QH);
	}
	else {
		ptd->Link = MM_GetPhysAddr(TD);
		LOG("Appended %p to end of %p", TD, QH);
	}
}

tEHCI_QH *EHCI_int_AllocateQH(tEHCI_Controller *Cont, int Endpoint, size_t MaxPacketSize)
{
	tEHCI_QH	*ret;
	Mutex_Acquire(&Cont->QHPoolMutex);
	for( int i = 0; i < QH_POOL_SIZE; i ++ )
	{
		if( !MM_GetPhysAddr( Cont->QHPools[i/QH_POOL_NPERPAGE] ) ) {
			tPAddr	tmp;
			Cont->QHPools[i/QH_POOL_NPERPAGE] = (void*)MM_AllocDMA(1, 32, &tmp);
			memset(Cont->QHPools[i/QH_POOL_NPERPAGE], 0, PAGE_SIZE);
		}

		ret = &Cont->QHPools[i/QH_POOL_NPERPAGE][i%QH_POOL_NPERPAGE];
		if( ret->HLink == 0 ) {
			ret->HLink = 1;
			ret->Overlay.Link = 1;
			ret->Endpoint = (Endpoint >> 4) | 0x80 | ((Endpoint & 0xF) << 8)
				| (MaxPacketSize << 16);
			// TODO: Endpoint speed (13:12) 0:Full, 1:Low, 2:High
			// TODO: Control Endpoint Flag (27) 0:*, 1:Full/Low Control
			Mutex_Release(&Cont->QHPoolMutex);
			return ret;
		}
	}
	Mutex_Release(&Cont->QHPoolMutex);
	return NULL;
}

void EHCI_int_DeallocateQH(tEHCI_Controller *Cont, tEHCI_QH *QH)
{
	// TODO: Ensure it's unused (somehow)
	QH->HLink = 0;
}

void EHCI_int_HandlePortConnectChange(tEHCI_Controller *Cont, int Port)
{
	// Connect Event
	if( Cont->OpRegs->PortSC[Port] & PORTSC_CurrentConnectStatus )
	{
		// Is the device low-speed?
		if( (Cont->OpRegs->PortSC[Port] & PORTSC_LineStatus_MASK) == PORTSC_LineStatus_Kstate )
		{
			LOG("Low speed device on %p Port %i, giving to companion", Cont, Port);
			Cont->OpRegs->PortSC[Port] |= PORTSC_PortOwner;
		}
		// not a low-speed device, EHCI reset
		else
		{
			LOG("Device connected on %p #%i", Cont, Port);
			// Reset procedure.
			USB_PortCtl_BeginReset(Cont->RootHub, Port);
		}
	}
	// Disconnect Event
	else
	{
		if( Cont->OpRegs->PortSC[Port] & PORTSC_PortOwner ) {
			LOG("Companion port %i disconnected, taking it back", Port);
			Cont->OpRegs->PortSC[Port] &= ~PORTSC_PortOwner;
		}
		else {
			LOG("Port %i disconnected", Port);
			USB_DeviceDisconnected(Cont->RootHub, Port);
		}
	}
}

void EHCI_int_InterruptThread(void *ControllerPtr)
{
	tEHCI_Controller	*Cont = ControllerPtr;
	while(Cont->OpRegs)
	{
		Uint32	events;
		
		LOG("sleeping for events");
		events = Threads_WaitEvents(EHCI_THREADEVENT_IOC|EHCI_THREADEVENT_PORTSC);
		if( !events ) {
			// TODO: Should this cause a termination?
		}
		LOG("events = 0x%x", events);

		if( events & EHCI_THREADEVENT_IOC )
		{
			// IOC, Do whatever it is you do
		}

		// Port status change interrupt
		if( events & EHCI_THREADEVENT_PORTSC )
		{
			// Check for port status changes
			for(int i = 0; i < Cont->nPorts; i ++ )
			{
				Uint32	sts = Cont->OpRegs->PortSC[i];
				LOG("Port %i: sts = %x", i, sts);
				Cont->OpRegs->PortSC[i] = sts;
				if( sts & PORTSC_ConnectStatusChange )
					EHCI_int_HandlePortConnectChange(Cont, i);

				if( sts & PORTSC_PortEnableChange )
				{
					// Handle enable/disable
				}

				if( sts & PORTSC_OvercurrentChange )
				{
					// Handle over-current change
				}
			}
		}

		LOG("Going back to sleep");
	}
}
