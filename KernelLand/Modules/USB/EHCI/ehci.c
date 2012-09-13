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

// === CONSTANTS ===
#define EHCI_MAX_CONTROLLERS	4

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
// --- Internals ---
tEHCI_qTD	*EHCI_int_AllocateTD(tEHCI_Controller *Cont, int PID, void *Data, size_t Length, tUSBHostCb Cb, void *CbData);
void	EHCI_int_DeallocateTD(tEHCI_Controller *Cont, tEHCI_qTD *TD);
void	EHCI_int_AppendTD(tEHCI_QH *QH, tEHCI_qTD *TD);
tEHCI_QH	*EHCI_int_AllocateQH(tEHCI_Controller *Cont, int Endpoint, size_t MaxPacketSize);
void	EHCI_int_DeallocateQH(tEHCI_Controller *Cont, tEHCI_QH *QH);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, USB_EHCI, EHCI_Initialise, NULL, "USB_Core", NULL);
tEHCI_Controller	gaEHCI_Controllers[EHCI_MAX_CONTROLLERS];
tUSBHostDef	gEHCI_HostDef = {
	.InitInterrupt = EHCI_InitInterrupt,
	.InitIsoch     = EHCI_InitIsoch,
	.InitBulk      = EHCI_InitBulk,
	.RemEndpoint   = EHCI_RemEndpoint,
	.SendIsoch   = NULL,
	.SendControl = EHCI_SendControl,
	.SendBulk    = EHCI_SendBulk,
	.FreeOp      = EHCI_FreeOp
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
		Log_Notice("EHCI", "Too many controllers (EHCI_MAX_CONTROLLERS=%i)", EHCI_MAX_CONTROLLERS);
		return 1;
	}

	// -- Build up structure --
	cont->CapRegs = (void*)MM_MapHWPages(BaseAddress, 1);
	// TODO: Error check
	cont->OpRegs = (void*)( (Uint32*)cont->CapRegs + cont->CapRegs->CapLength / 4 );
	// - Allocate periodic queue
	cont->PeriodicQueue = (void*)MM_AllocDMA(1, 32, NULL);
	// TODO: Error check
	//  > Populate queue

	// -- Bind IRQ --
	IRQ_AddHandler(InterruptNum, EHCI_InterruptHandler, cont);

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
	
	return 0;
}

void EHCI_InterruptHandler(int IRQ, void *Ptr)
{
	tEHCI_Controller *cont = Ptr;
	Uint32	sts = cont->OpRegs->USBSts;
	
	// Clear interrupts
	cont->OpRegs->USBSts = sts;	

	if( sts & 0xFFFF0FC0 ) {
		LOG("Oops, reserved bits set (%08x), funny hardware?", sts);
		sts &= ~0xFFFF0FFC0;
	}

	// Unmask read-only bits
	sts &= ~(0xF000);

	if( sts & USBINTR_IOC ) {
		// IOC
		sts &= ~USBINTR_IOC;
	}

	if( sts & USBINTR_PortChange ) {
		// Port change, determine what port and poke helper thread
		LOG("Port status change");
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
	for( int i = 0; i < PERIODIC_SIZE; i += Period )
		Cont->InterruptLoad[i+minslot] += Length;
	qh->Impl.IntOfs = minslot;

	// Allocate TD for the data
	tEHCI_qTD *td = EHCI_int_AllocateTD(Cont, (bOutbound ? PID_OUT : PID_IN), Buf, Length, Cb, CbData);
	EHCI_int_AppendTD(qh, td);

	// Insert into the periodic list

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

	// Append to async list	
	if( Cont->LastAsyncHead ) {
		Cont->LastAsyncHead->HLink = MM_GetPhysAddr(qh)|2;
		Cont->LastAsyncHead->Impl.Next = qh;
	}
	else
		Cont->OpRegs->AsyncListAddr = MM_GetPhysAddr(qh)|2;
	Cont->LastAsyncHead = qh;

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
		// Remove QH from list
		// - If it's a polling endpoint, need to remove from all periodic lists
		
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
		td_data = EHCI_int_AllocateTD(Cont, PID_OUT, (void*)OutData, OutLength, NULL, NULL);
		td_status = EHCI_int_AllocateTD(Cont, PID_IN, InData, InLength, Cb, CbData);
	}
	else
	{
		td_data = EHCI_int_AllocateTD(Cont, PID_IN, InData, InLength, NULL, NULL);
		td_status = EHCI_int_AllocateTD(Cont, PID_OUT, (void*)OutData, OutLength, Cb, CbData);
	}

	// Append TDs
	EHCI_int_AppendTD(Dest, td_setup);
	EHCI_int_AppendTD(Dest, td_data);
	EHCI_int_AppendTD(Dest, td_status);

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
	EHCI_int_AppendTD(Dest, td);	

	return td;
}

void EHCI_FreeOp(void *Ptr, void *Handle)
{
	tEHCI_Controller	*Cont = Ptr;

	EHCI_int_DeallocateTD(Cont, Handle);
}

// --------------------------------------------------------------------
// Internals
// --------------------------------------------------------------------
tEHCI_qTD *EHCI_int_AllocateTD(tEHCI_Controller *Cont, int PID, void *Data, size_t Length, tUSBHostCb Cb, void *CbData)
{
	return NULL;
}

void EHCI_int_DeallocateTD(tEHCI_Controller *Cont, tEHCI_qTD *TD)
{
}

void EHCI_int_AppendTD(tEHCI_QH *QH, tEHCI_qTD *TD)
{
}

tEHCI_QH *EHCI_int_AllocateQH(tEHCI_Controller *Cont, int Endpoint, size_t MaxPacketSize)
{
	return NULL;
}

void EHCI_int_DeallocateQH(tEHCI_Controller *Cont, tEHCI_QH *QH)
{
}

