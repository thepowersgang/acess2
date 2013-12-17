/*
 * Acess2 EHCI Driver
 * - By John Hodge (thePowersGang)
 * 
 * ehci.c
 * - Driver Core
 */
#define DEBUG	0
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
#define EHCI_THREADEVENT_IAAD	THREAD_EVENT_USER3

// === PROTOTYPES ===
 int	EHCI_Initialise(char **Arguments);
 int	EHCI_Cleanup(void);
 int	EHCI_InitController(tPAddr BaseAddress, Uint8 InterruptNum);
// -- API ---
void	*EHCI_InitInterrupt(void *Ptr, int Endpoint, int bInput, int Period,
	tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
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
tEHCI_Endpoint	*EHCI_int_AllocateEndpt(tEHCI_Controller *Ctrlr, int Endpoint, size_t MPS, int PeriodPow2);
tEHCI_qTD	*EHCI_int_AllocateTD(tEHCI_Controller *Cont, int PID, void *Data, size_t Length);
void	EHCI_int_DeallocateTD(tEHCI_Controller *Cont, tEHCI_qTD *TD);
tEHCI_QH	*EHCI_int_AllocateQH(tEHCI_Controller *Cont, tEHCI_Endpoint *Endpoint, tUSBHostCb Cb, void *CbPtr);
void	EHCI_int_DeallocateQH(tEHCI_Controller *Cont, tEHCI_QH *QH);
void 	EHCI_InterruptHandler(int IRQ, void *Ptr);
// --- Interrupts ---
void	EHCI_int_AppendQHToAsync(tEHCI_Controller *Cont, tEHCI_QH *QH);
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
		Uint32	addr = PCI_GetValidBAR(id, 0, PCI_BARTYPE_MEM);
		if( addr == 0 ) {
			Log_Error("EHCI", "PCI%i BAR0 is not memory", id);
			continue ;
		}
		Uint8	irq = PCI_GetIRQ(id);
		if( irq == 0 ) {
			Log_Error("EHCI", "PCI%i has no IRQ", id);
			continue ;
		}

		Log_Log("ECHI", "Controller at PCI %i 0x%x IRQ 0x%x", id, addr, irq);

		if( EHCI_InitController(addr, irq) )
		{
			// TODO: Detect other forms of failure than "out of slots"
			break ;
		}
	}

	for( int i = 0; Arguments && Arguments[i]; i ++ )
	{
		char *pos = Arguments[i], *next;
		LOG("pos = '%s'", pos);
		tPAddr base = strtoull(pos, &next, 16);
		if( base == 0 )
			continue;
		pos = next;
		LOG("pos = '%s'", pos);
		if( *pos++ != '-' )
			continue;
		LOG("pos = '%s'", pos);
		int irq = strtol(pos, &next, 16);
		if( irq == 0 )
			continue ;
		if( *next != 0 )
			continue;
		LOG("base=%x, irq=%i", base, irq);
		if( EHCI_InitController(base, irq) )
		{
			continue ;
		}
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

	ENTER("PBaseAddress iInterruptNum",
		BaseAddress, InterruptNum);

	// Allocate a controller structure
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
		LEAVE('i', 1);
		return 1;
	}

	// - Nuke a couple of fields so error handling code doesn't derp
	cont->CapRegs = NULL;
	cont->PeriodicQueue = NULL;
	cont->TDPool = NULL;

	// -- Build up structure --
	cont->CapRegs = (void*)( MM_MapHWPages(BaseAddress, 1) + (BaseAddress % PAGE_SIZE) );
	if( !cont->CapRegs ) {
		Log_Warning("EHCI", "Can't map 1 page at %P into kernel space", BaseAddress);
		goto _error;
	}
	LOG("cont->CapRegs = %p", cont->CapRegs);
	if( (cont->CapRegs->CapLength & 3) ) {
		Log_Warning("EHCI", "Controller at %P non-aligned op regs (%x)",
			BaseAddress, cont->CapRegs->CapLength);
		goto _error;
	}
	if( BaseAddress % PAGE_SIZE + cont->CapRegs->CapLength + sizeof(tEHCI_CapRegs) > PAGE_SIZE ) {
		Log_Warning("EHCI", "%P: Cap regs over page boundary (+0x%x bytes)",
			 BaseAddress % PAGE_SIZE + cont->CapRegs->CapLength + sizeof(tEHCI_CapRegs)
			 );
		goto _error;
	}
	cont->OpRegs = (void*)( (Uint32*)cont->CapRegs + cont->CapRegs->CapLength / 4 );
	LOG("cont->OpRegs = %p", cont->OpRegs);
	// - Allocate periodic queue
	tPAddr	unused;
	cont->PeriodicQueue = (void*)MM_AllocDMA(1, 32, &unused);
	if( !cont->PeriodicQueue ) {
		Log_Warning("ECHI", "Can't allocate 1 32-bit page for periodic queue");
		goto _error;
	}
	//  > Populate queue (with non-present entries)
	for( int i = 0; i < 1024; i ++ )
		cont->PeriodicQueue[i] = 1;
	// TODO: For 64-bit, QH pool and periodic queue need to be in the same 32-bit segment

	// - Allocate TD pool
	cont->TDPool = (void*)MM_AllocDMA(1, 32, &unused);
	if( !cont->TDPool ) {
		Log_Warning("ECHI", "Can't allocate 1 32-bit page for qTD pool");
		goto _error;
	}
	for( int i = 0; i < TD_POOL_SIZE; i ++ ) {
		cont->TDPool[i].Token = 3 << 8;	// TODO: what is this value?
	}

	// Get port count
	cont->nPorts = cont->CapRegs->HCSParams & 0xF;
	LOG("cont->nPorts = %i", cont->nPorts);

	// -- Bind IRQ --
	IRQ_AddHandler(InterruptNum, EHCI_InterruptHandler, cont);
	cont->InterruptThread = Proc_SpawnWorker(EHCI_int_InterruptThread, cont);
	if( !cont->InterruptThread ) {
		Log_Warning("EHCI", "Can't spawn interrupt worker thread");
		goto _error;
	}
	LOG("cont->InterruptThread = %p", cont->InterruptThread);

	// Dummy TD
	cont->DeadTD = EHCI_int_AllocateTD(cont, 0, NULL, 0);
	memset(cont->DeadTD, 0, sizeof(tEHCI_qTD));
	cont->DeadTD->Link = 1;
	cont->DeadTD->Link2 = 1;
	cont->DeadTD->Token = QTD_TOKEN_STS_HALT;
	
	// Dummy QH
	// - HLink is set to itself, initing a circular list
	cont->DeadQH = EHCI_int_AllocateQH(cont, NULL, NULL, NULL);
	memset(cont->DeadQH, 0, sizeof(tEHCI_QH));
	cont->DeadQH->HLink = MM_GetPhysAddr(cont->DeadQH)|2;
	cont->DeadQH->Impl.Next = cont->DeadQH;
	cont->DeadQH->Endpoint = QH_ENDPT_H;	// H - Head of Reclamation List
	cont->DeadQH->CurrentTD = MM_GetPhysAddr(cont->DeadTD);
	cont->DeadQH->Overlay.Link = MM_GetPhysAddr(cont->DeadTD);

	// -- Initialisation procedure (from ehci-r10) --
	// - Reset controller
	cont->OpRegs->USBCmd = USBCMD_HCReset;
	// - Set CTRLDSSEGMENT (TODO: 64-bit support)
	// - Set USBINTR
	cont->OpRegs->USBIntr = USBINTR_IOC|USBINTR_PortChange|USBINTR_FrameRollover|USBINTR_IntrAsyncAdvance;
	// - Set PERIODICLIST BASE
	cont->OpRegs->PeridocListBase = MM_GetPhysAddr( cont->PeriodicQueue );
	// - Set ASYNCLISTADDR
	cont->OpRegs->AsyncListAddr = MM_GetPhysAddr(cont->DeadQH);
	// - Enable controller
	cont->OpRegs->USBCmd = (0x40 << 16) | USBCMD_PeriodicEnable | USBCMD_AsyncEnable | USBCMD_Run;
	// - Route all ports
	cont->OpRegs->ConfigFlag = 1;

	// -- Register with USB Core --
	cont->RootHub = USB_RegisterHost(&gEHCI_HostDef, cont, cont->nPorts);
	LOG("cont->RootHub = %p", cont->RootHub);

	LEAVE('i', 0);
	return 0;
_error:
	cont->PhysBase = 0;
	if( cont->CapRegs )
		MM_Deallocate( (tVAddr)cont->CapRegs );
	if( cont->PeriodicQueue )
		MM_Deallocate( (tVAddr)cont->PeriodicQueue );
	if( cont->TDPool )
		MM_Deallocate( (tVAddr)cont->TDPool );
	LEAVE('i', 2);
	return 2;
}


static inline int _GetClosestPower2(int Period)
{
	// Round the period to the closest power of two
	int pow2period = 1;
	int period_pow = 0;
	// - Find the first power above the period
	while( pow2period < Period )
	{
		pow2period *= 2;
		period_pow ++;
	}
	// - Check which is closest
	if( Period - pow2period / 2 > pow2period - Period ) {
		;
	}
	else {
		period_pow --;
	}
	LOG("period_pow = %i (%ims) from %ims", period_pow, 1 << period_pow, Period);
	return period_pow;
}

void EHCI_int_AddToPeriodic(tEHCI_Controller *Cont, tEHCI_QH *qh, int PeriodPow2, size_t Load)
{
	 int	Period = 1 << PeriodPow2;
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
		Cont->InterruptLoad[i] += Load;
	qh->Impl.Endpt->InterruptOfs = minslot;
	LOG("Using slot %i", minslot);

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
			if( nqh->Impl.Endpt->PeriodPow2 < PeriodPow2 )
				break;
		}

		// Somehow, we've already been added to this queue.
		if( nqh && nqh == qh )
			continue ;

		if( qh->Impl.Next && qh->Impl.Next != nqh ) {
			Log_Warning("EHCI", "Suspected bookkeeping error on %p - int list %i+%i overlap",
				Cont, PeriodPow2, minslot);
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
}

// --------------------------------------------------------------------
// USB API
// --------------------------------------------------------------------
void *EHCI_InitInterrupt(void *Ptr, int Endpoint, int bOutbound, int Period,
	tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	tEHCI_Controller	*Cont = Ptr;
	
	ASSERTCR(Endpoint, <, 256*16, NULL);
	ASSERTCR(Period, >, 0, NULL);
	if( Period > 256 )
		Period = 256;

	LOG("Endpoint=%x, bOutbound=%i, Period=%i, Length=%i", Endpoint, bOutbound, Period, Length);

	int period_pow = _GetClosestPower2(Period);

	tEHCI_Endpoint	*endpt = EHCI_int_AllocateEndpt(Cont, Endpoint, Length, period_pow);
	endpt->Next = Cont->FirstInterruptEndpt;
	Cont->FirstInterruptEndpt = endpt;

	// Allocate a QH
	tEHCI_QH *qh = EHCI_int_AllocateQH(Cont, endpt, Cb, CbData);
	qh->EndpointExt |= 1;	// TODO: uFrame load balancing (8 entry bitfield)

	// Allocate TD for the data
	tEHCI_qTD *td = EHCI_int_AllocateTD(Cont, (bOutbound ? PID_OUT : PID_IN), Buf, Length);
	qh->CurrentTD = MM_GetPhysAddr(td);
	qh->Impl.FirstTD = td;
	qh->Impl.LastTD = td;

	// - Insert into period queue
	EHCI_int_AddToPeriodic(Cont, qh, period_pow, Length);

	return endpt;
}

void *EHCI_InitIsoch(void *Ptr, int Endpoint, size_t MaxPacketSize)
{
	ENTER("pPtr iEndpoint iMaxPacketSize",
		Ptr, Endpoint, MaxPacketSize);
	LEAVE_RET('p', (void*)(tVAddr)(Endpoint + 1));
}
void *EHCI_InitControl(void *Ptr, int Endpoint, size_t MaxPacketSize)
{
	return EHCI_int_AllocateEndpt(Ptr, Endpoint, MaxPacketSize, -1);
}
void *EHCI_InitBulk(void *Ptr, int Endpoint, size_t MaxPacketSize)
{
	return EHCI_int_AllocateEndpt(Ptr, Endpoint, MaxPacketSize, -2);
}
void EHCI_RemEndpoint(void *Ptr, void *Handle)
{
	if( Handle == NULL )
		return ;
	else if( (tVAddr)Handle <= 256*16 )
		return ;	// Isoch
	else {
		tEHCI_Endpoint	*endpt = Ptr;

		// Remove QH from list
		// - If it's a polling endpoint, need to remove from all periodic lists
		if( endpt->PeriodPow2 >= 0) {
			// Poll
		}
		else {
			// Control/Bulk
		}
		
		// Deallocate endpoint
		// TODO: Write EHCI_DeallocateEndpoint
		free(endpt);
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
	tEHCI_Endpoint	*endpt = Dest;
	tEHCI_QH	*qh;
	tEHCI_qTD	*td_setup, *td_data, *td_status;

	// Sanity checks
	if( (tVAddr)Dest <= 256*16 )
		return NULL;
	if( endpt->PeriodPow2 != -1 ) {
		Log_Notice("EHCI", "Non-control endpoint passed to SendControl");
		return NULL;
	}
	if( SetupLength > endpt->MaxPacketSize )
		return NULL;
	// TODO: Check size of status
	

	LOG("Dest=%p, isOutbound=%i, Lengths(Setup:%i,Out:%i,In:%i)",
		Dest, isOutbound, SetupLength, OutLength, InLength);

	// Allocate a QH to work with
	qh = EHCI_int_AllocateQH(Cont, endpt, Cb, CbData);

	// Allocate TDs
	td_setup = EHCI_int_AllocateTD(Cont, PID_SETUP, (void*)SetupData, SetupLength);
	if( isOutbound )
	{
		td_data = OutData ? EHCI_int_AllocateTD(Cont, PID_OUT, (void*)OutData, OutLength) : NULL;
		td_status = EHCI_int_AllocateTD(Cont, PID_IN, InData, InLength);
	}
	else
	{
		td_data = InData ? EHCI_int_AllocateTD(Cont, PID_IN, InData, InLength) : NULL;
		td_status = EHCI_int_AllocateTD(Cont, PID_OUT, (void*)OutData, OutLength);
	}
	td_status->Token |= QTD_TOKEN_IOC;	// IOC

	// Append TDs
	if( td_data ) {
		td_setup->Link = MM_GetPhysAddr(td_data);
		td_data->Link = MM_GetPhysAddr(td_status);
		td_data->Token |= QTD_TOKEN_STS_ACTIVE;	// Active
	}
	else {
		td_setup->Link = MM_GetPhysAddr(td_status);
	}
	td_setup->Token |= QTD_TOKEN_STS_ACTIVE;	// Active
	td_status->Token |= QTD_TOKEN_STS_ACTIVE;
	td_status->Link = 1;
	td_status->Link2 = 1;
	
	// Set QH's current pointer
	qh->CurrentTD = MM_GetPhysAddr(td_setup);
	qh->Impl.FirstTD = td_setup;
	qh->Impl.LastTD = td_status;
	EHCI_int_AppendQHToAsync(Cont, qh);

	return qh;
}

void *EHCI_SendBulk(void *Ptr, void *Dest, tUSBHostCb Cb, void *CbData, int Dir, void *Data, size_t Length)
{
	tEHCI_Controller	*Cont = Ptr;
	tEHCI_Endpoint	*endpt = Dest;
	
	// Sanity check the pointer
	// - Can't be NULL or an isoch
	if( (tVAddr)Dest <= 256*16 )
		return NULL;
	if( endpt->PeriodPow2 != -2 ) {
		return NULL;
	}
	LOG("Ptr=%p,Dest=%p(%x),Cb=%p(%p),DirIsOut=%i,Data=%p+0x%x)",
		Ptr, Dest, endpt->EndpointID, Cb, CbData, Dir, Data, Length);

	// Allocate a QH to work with
	tEHCI_QH *qh = EHCI_int_AllocateQH(Cont, endpt, Cb, CbData);
	
	// Allocate single TD
	tEHCI_qTD *td = EHCI_int_AllocateTD(Cont, (Dir ? PID_OUT : PID_IN), Data, Length);
	td->Token |= QTD_TOKEN_IOC | QTD_TOKEN_STS_ACTIVE;
	qh->CurrentTD = MM_GetPhysAddr(td);
	qh->Impl.FirstTD = td;
	qh->Impl.LastTD = td;
	
	EHCI_int_AppendQHToAsync(Cont, qh);

	return qh;
}

void EHCI_FreeOp(void *Ptr, void *Handle)
{
	tEHCI_Controller	*Cont = Ptr;

	EHCI_int_DeallocateQH(Cont, Handle);
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
tEHCI_Endpoint *EHCI_int_AllocateEndpt(tEHCI_Controller *Cont, int Endpoint, size_t MaxPacketSize, int PeriodPow2)
{
	ENTER("pCont iEndpoint iMaxPacketSize",
		Cont, Endpoint, MaxPacketSize);
	
	tEHCI_Endpoint	*endpt = malloc( sizeof(*endpt) );
	endpt->NextToggle = FALSE;
	endpt->EndpointID = Endpoint;
	endpt->MaxPacketSize = MaxPacketSize;	// TODO: Sanity Check
	endpt->PeriodPow2 = PeriodPow2;
	endpt->InterruptOfs = 0;
	endpt->ActiveQHs = NULL;
	// TODO: store endpoints on controller
	
	LOG("Created %p for %P Ep 0x%x - %i bytes MPS",
		endpt, Cont->PhysBase, Endpoint, MaxPacketSize);

	LEAVE('p', endpt);
	return endpt;
}

tEHCI_qTD *EHCI_int_GetTDFromPhys(tEHCI_Controller *Cont, Uint32 Addr)
{
	if( Addr == 0 )	return NULL;
	LOG("%p + (%x - %x)", Cont->TDPool, Addr, MM_GetPhysAddr(Cont->TDPool));
	return Cont->TDPool + (Addr - MM_GetPhysAddr(Cont->TDPool))/sizeof(tEHCI_qTD);
}

tEHCI_qTD *EHCI_int_AllocateTD(tEHCI_Controller *Cont, int PID, void *Data, size_t Length)
{
//	Semaphore_Wait(&Cont->TDSemaphore, 1);
	Mutex_Acquire(&Cont->TDPoolMutex);
	for( int i = 0; i < TD_POOL_SIZE; i ++ )
	{
		// PID code == 3: Avaliable for use
		if( ((Cont->TDPool[i].Token >> 8) & 3) != 3 )
			continue ;
		
		Cont->TDPool[i].Token = (PID << 8) | (Length << 16);
		Mutex_Release(&Cont->TDPoolMutex);
		
		tEHCI_qTD	*td = &Cont->TDPool[i];
		td->Impl.Ptr = Data;
		td->Impl.Length = Length;
		td->Link = 1;
		td->Link2 = 1;
		
		// TODO: Support breaking across multiple pages
		ASSERTC(Length, <, PAGE_SIZE);
		// TODO: Handle bouncing >32-bit pages
		#if PHYS_BITS > 32
		ASSERT( MM_GetPhysAddr(Data) >> 32 == 0 );
		#endif
		td->Pages[0] = MM_GetPhysAddr(Data);
		if( (td->Pages[0] & (PAGE_SIZE-1)) + Length - 1 > PAGE_SIZE )
			td->Pages[1] = MM_GetPhysAddr((char*)Data + Length - 1) & ~(PAGE_SIZE-1);
		LOG("Allocated %p(%P) PID%i on %P (Token=0x%x)",
			td, MM_GetPhysAddr(td), PID, Cont->PhysBase,
			td->Token);
		return td;
	}

	Mutex_Release(&Cont->TDPoolMutex);
	return NULL;
}

void EHCI_int_DeallocateTD(tEHCI_Controller *Cont, tEHCI_qTD *TD)
{
	UNIMPLEMENTED();
}

tEHCI_QH *EHCI_int_AllocateQH(tEHCI_Controller *Cont, tEHCI_Endpoint *Endpoint, tUSBHostCb Cb, void *CbData)
{
	tEHCI_QH	*ret;
	Mutex_Acquire(&Cont->QHPoolMutex);
	for( int i = 0; i < QH_POOL_SIZE; i ++ )
	{
		// If page not yet allocated, allocate it
		if( !MM_GetPhysAddr( Cont->QHPools[i/QH_POOL_NPERPAGE] ) ) {
			tPAddr	tmp;
			Cont->QHPools[i/QH_POOL_NPERPAGE] = (void*)MM_AllocDMA(1, 32, &tmp);
			memset(Cont->QHPools[i/QH_POOL_NPERPAGE], 0, PAGE_SIZE);
		}

		ret = &Cont->QHPools[i/QH_POOL_NPERPAGE][i%QH_POOL_NPERPAGE];
		if( ret->HLink != 0 )
			continue ;
		
		memset(ret, 0, sizeof(*ret));
		ret->HLink = 1;
		Mutex_Release(&Cont->QHPoolMutex);
		
		if( Endpoint )
		{
			ret->Endpoint = (Endpoint->EndpointID >> 4) | 0x80 | ((Endpoint->EndpointID & 0xF) << 8)
				| (Endpoint->MaxPacketSize << 16);
		}
		ret->EndpointExt = (1<<30);
		ret->Impl.Next = NULL;
		ret->Impl.Callback = Cb;
		ret->Impl.CallbackData = CbData;
		ret->Impl.Endpt = Endpoint;
		// TODO: Endpoint speed (13:12) 0:Full, 1:Low, 2:High
		// TODO: Control Endpoint Flag (27) 0:*, 1:Full/Low Control
		return ret;
	}
	Mutex_Release(&Cont->QHPoolMutex);
	return NULL;
}

void EHCI_int_DeallocateQH(tEHCI_Controller *Cont, tEHCI_QH *QH)
{
	UNIMPLEMENTED();
	// TODO: Ensure it's unused and clean up associated TDs
	QH->HLink = 0;
}

void EHCI_int_AppendQHToAsync(tEHCI_Controller *Cont, tEHCI_QH *QH)
{
	QH->Overlay.Token = QTD_TOKEN_STS_ACTIVE;
	
	Mutex_Acquire(&Cont->lAsyncSchedule);
	// Insert into list
	QH->HLink = Cont->DeadQH->HLink;
	QH->Impl.Next = Cont->DeadQH->Impl.Next;
	Cont->DeadQH->HLink = MM_GetPhysAddr(QH)|2;
	Cont->DeadQH->Impl.Next = QH;
	// Start async schedule
	// - Set ASYNCENABLE
	// - Clear 'H' in dead QH
	Mutex_Release(&Cont->lAsyncSchedule);
	LOG("Appended %P to %p(%P)", MM_GetPhysAddr(QH), Cont, Cont->PhysBase);
}

// --------------------------------------------------------------------
// Interrupt actions
// --------------------------------------------------------------------
void EHCI_InterruptHandler(int IRQ, void *Ptr)
{
	tEHCI_Controller *Cont = Ptr;
	Uint32	sts = Cont->OpRegs->USBSts;
	Uint32	orig_sts = sts;
	const Uint32	reserved_bits = 0xFFFF0FC0;

	if( sts & reserved_bits ) {
		LOG("Oops, reserved bits set (%08x), funny hardware?", sts);
		sts &= ~reserved_bits;
	}

	// Unmask read-only bits
	sts &= ~(0xF000);

	if( sts & USBINTR_IOC ) {
		// IOC
		LOG("%P IOC", Cont->PhysBase);
		Threads_PostEvent(Cont->InterruptThread, EHCI_THREADEVENT_IOC);
		sts &= ~USBINTR_IOC;
	}

	if( sts & USBINTR_IntrAsyncAdvance ) {
		LOG("%P IAAD", Cont->PhysBase);
		Threads_PostEvent(Cont->InterruptThread, EHCI_THREADEVENT_IAAD);
		sts &= ~USBINTR_IntrAsyncAdvance;
	}

	if( sts & USBINTR_PortChange ) {
		// Port change, determine what port and poke helper thread
		LOG("%P Port status change", Cont->PhysBase);
		Threads_PostEvent(Cont->InterruptThread, EHCI_THREADEVENT_PORTSC);
		sts &= ~USBINTR_PortChange;
	}
	
	if( sts & USBINTR_FrameRollover ) {
		// Frame rollover, used to aid timing (trigger per-second operations)
		//LOG("%p Frame rollover", Ptr);
		sts &= ~USBINTR_FrameRollover;
	}

	if( sts ) {
		// Unhandled interupt bits
		// TODO: Warn
		LOG("WARN - Bitmask %x unhandled", sts);
	}


	// Clear interrupts
	Cont->OpRegs->USBSts = orig_sts;
}

void EHCI_int_HandlePortConnectChange(tEHCI_Controller *Cont, int Port)
{
	// Connect Event
	if( Cont->OpRegs->PortSC[Port] & PORTSC_CurrentConnectStatus )
	{
		// Is the device low-speed?
		if( (Cont->OpRegs->PortSC[Port] & PORTSC_LineStatus_MASK) == PORTSC_LineStatus_Kstate )
		{
			LOG("Low speed device on %P Port %i, giving to companion", Cont->PhysBase, Port);
			Cont->OpRegs->PortSC[Port] |= PORTSC_PortOwner;
		}
		// not a low-speed device, EHCI reset
		else
		{
			LOG("Device connected on %P Port %i", Cont->PhysBase, Port);
			// Reset procedure.
			USB_PortCtl_BeginReset(Cont->RootHub, Port);
		}
	}
	// Disconnect Event
	else
	{
		if( Cont->OpRegs->PortSC[Port] & PORTSC_PortOwner ) {
			LOG("Device disconnected on %P Port %i (companion), taking it back",
				Cont->PhysBase, Port);
			Cont->OpRegs->PortSC[Port] &= ~PORTSC_PortOwner;
		}
		else {
			LOG("Device disconnected on %P Port %i", Cont->PhysBase, Port);
			USB_DeviceDisconnected(Cont->RootHub, Port);
		}
	}
}

void EHCI_int_CheckInterruptQHs(tEHCI_Controller *Cont)
{
	for( tEHCI_Endpoint *endpt = Cont->FirstInterruptEndpt; endpt; endpt = endpt->Next )
	{
		tEHCI_QH *qh = endpt->ActiveQHs;
		// Check if the TD of the first QH is active
		if( qh->Overlay.Token & QTD_TOKEN_STS_ACTIVE )
			continue ;
		// Inactive, fire interrupt and re-trigger
		if( !qh->Impl.Callback ) {
			// Umm... ?
		}
		else
		{
			tEHCI_qTD	*last = qh->Impl.LastTD;
			size_t	remaining_len = (last->Token >> 16) & 0x7FFF;
			if( remaining_len > last->Impl.Length )
				remaining_len = last->Impl.Length;
			size_t	transferred_len = last->Impl.Length - remaining_len;
			
			LOG("Calling %p(%p) for Intr EndPt %x (%p+0x%x)",
				qh->Impl.Callback, qh->Impl.CallbackData,
				qh->Impl.Endpt->EndpointID, last->Impl.Ptr, transferred_len);
			qh->Impl.Callback(qh->Impl.CallbackData, last->Impl.Ptr, transferred_len);
		}
		qh->Impl.FirstTD->Token |= QTD_TOKEN_STS_ACTIVE;
		qh->Overlay.Token |= QTD_TOKEN_STS_ACTIVE;
	}
}

void EHCI_int_RetireQHs(tEHCI_Controller *Cont)
{
	tEHCI_QH	*prev = Cont->DeadQH;
	Mutex_Acquire(&Cont->lAsyncSchedule);
	for( tEHCI_QH *qh = prev->Impl.Next; qh != Cont->DeadQH; qh = prev->Impl.Next )
	{
		ASSERT(qh);
		ASSERT(qh != qh->Impl.Next);
		if( qh->Overlay.Token & QTD_TOKEN_STS_ACTIVE ) {
			prev = qh;
			continue ;
		}
		
		// Remove from async list
		prev->HLink = qh->HLink;
		prev->Impl.Next = qh->Impl.Next;
		
		// Add to reclaim list
		qh->Impl.Next = Cont->ReclaimList;
		Cont->ReclaimList = qh;
		
		// Ring doorbell!
		Cont->OpRegs->USBCmd |= USBCMD_IAAD;
	}
	Mutex_Release(&Cont->lAsyncSchedule);
}

/*
 * Fire callbacks on QHs and mark them as completed
 * 
 * TODO: Possible bug with items being added to reclaim list before
 *       the last doorbell fires.
 */
void EHCI_int_ReclaimQHs(tEHCI_Controller *Cont)
{
	// Doorbell was rung, so reclaim QHs
	// - Actually just fires callbacks, now that we know that the QHs can be cleared
	tEHCI_QH	*qh;
	Mutex_Acquire(&Cont->lReclaimList);
	while( (qh = Cont->ReclaimList) )
	{
		Cont->ReclaimList = qh->Impl.Next;
		Mutex_Release(&Cont->lReclaimList);

		if( qh->Impl.Callback )
		{
			tEHCI_qTD	*last = qh->Impl.LastTD;
			size_t	remaining_len = (last->Token >> 16) & 0x7FFF;
			if( remaining_len > last->Impl.Length )
				remaining_len = last->Impl.Length;
			size_t	transferred_len = last->Impl.Length - remaining_len;
			
			LOG("Calling %p(%p) for EndPt %x (%p+0x%x)",
				qh->Impl.Callback, qh->Impl.CallbackData,
				qh->Impl.Endpt->EndpointID, last->Impl.Ptr, transferred_len);
			qh->Impl.Callback(qh->Impl.CallbackData, last->Impl.Ptr, transferred_len);
		}
		
		Mutex_Acquire(&Cont->lReclaimList);
	}
	Mutex_Release(&Cont->lReclaimList);
}

void EHCI_int_InterruptThread(void *ControllerPtr)
{
	tEHCI_Controller	*Cont = ControllerPtr;
	while(Cont->OpRegs)
	{
		Uint32	events;
		
		LOG("sleeping for events");
		events = Threads_WaitEvents(EHCI_THREADEVENT_IOC|EHCI_THREADEVENT_PORTSC|EHCI_THREADEVENT_IAAD);
		if( !events ) {
			// TODO: Should this cause a termination?
		}
		LOG("events = 0x%x", events);

		if( events & EHCI_THREADEVENT_IAAD )
		{
			EHCI_int_ReclaimQHs(Cont);
		}

		if( events & EHCI_THREADEVENT_IOC )
		{
			// IOC, handle completed requests
			Log_Warning("EHCI", "%P IOC - TODO: Call registered callbacks and reclaim",
				Cont->PhysBase);
			// Search periodic lists for one that fired
			// Retire QHs
			// - Remove them from the queue and ask the controller to bell when they're removable
			EHCI_int_RetireQHs(Cont);
		}

		// Port status change interrupt
		if( events & EHCI_THREADEVENT_PORTSC )
		{
			// Check for port status changes
			for(int i = 0; i < Cont->nPorts; i ++ )
			{
				Uint32	sts = Cont->OpRegs->PortSC[i];
				//LOG("Port %i: sts = %x", i, sts);
				Cont->OpRegs->PortSC[i] = sts;
				if( sts & PORTSC_ConnectStatusChange )
				{
					LOG("Port %i: Connect Change", i);
					EHCI_int_HandlePortConnectChange(Cont, i);
				}

				if( sts & PORTSC_PortEnableChange )
				{
					// Handle enable/disable
					LOG("Port %i: Enable Change", i);
				}

				if( sts & PORTSC_OvercurrentChange )
				{
					// Handle over-current change
					LOG("Port %i: Over-Current Change", i);
				}
			}
		}
	}
}
