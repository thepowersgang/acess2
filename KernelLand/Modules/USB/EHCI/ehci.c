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

// === CONSTANTS ===
#define EHCI_MAX_CONTROLLERS	4

// === PROTOTYPES ===
 int	EHCI_Initialise(char **Arguments);
 int	EHCI_Cleanup(void);
 int	EHCI_InitController(tPAddr BaseAddress, Uint8 InterruptNum);
void 	EHCI_InterruptHandler(int IRQ, void *Ptr);
// -- API ---
void	*EHCI_InitInterrupt(void *Ptr, int Endpoint, int bInput, int Period, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
void	*EHCI_InitIsoch  (void *Ptr, int Endpoint);
void	*EHCI_InitControl(void *Ptr, int Endpoint);
void	*EHCI_InitBulk   (void *Ptr, int Endpoint);
void	EHCI_RemEndpoint(void *Ptr, void *Handle);
void	*EHCI_SendControl(void *Ptr, void *Dest, tUSBHostCb Cb, void *CbData,
	int isOutbound,
	const void *SetupData, size_t SetupLength,
	const void *OutData, size_t OutLength,
	void *InData, size_t InLength
	);
void	*EHCI_SendBulk(void *Ptr, void *Dest, tUSBHostCb Cb, void *CbData, int Dir, void *Data, size_t Length);

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
	.FreeOp      = NULL
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

		if( EHCI_InitController(addr, irq) ) {
			// TODO: Detect other forms of failure than "out of slots"
			break ;
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

	for( int i = 0; i < EHCI_MAX_CONTROLLERS; i ++ )
	{
		if( gaEHCI_Controllers[i].PhysBase == 0 ) {
			cont = &gaEHCI_Controllers[i];
			cont->PhysBase = BaseAddress;
			break;
		}
	}

	if(!cont) {
		return 1;
	}

	// -- Build up structure --
	cont->CapRegs = (void*)MM_MapHWPages(BaseAddress, 1);
	// TODO: Error check
	cont->OpRegs = (void*)( (Uint32*)cont->CapRegs + cont->CapRegs->CapLength / 4 );
	// - Allocate periodic queue
	cont->PeriodicQueue = (void*)MM_AllocDMA(1, 32, NULL);
	// TODO: Error check

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

	if( sts & USBINTR_IOC ) {
		// IOC
		sts &= ~USBINTR_IOC;
	}

	if( sts & USBINTR_PortChange ) {
		// Port change, determine what port and poke helper thread
		sts &= ~USBINTR_PortChange;
	}
	
	if( sts & USBINTR_FrameRollover ) {
		// Frame rollover, used to aid timing (trigger per-second operations)
		sts &= ~USBINTR_FrameRollover;
	}

	if( sts ) {
		// Unhandled interupt bits
		// TODO: Warn
	}
}

// --------------------------------------------------------------------
// USB API
// --------------------------------------------------------------------
void *EHCI_InitInterrupt(void *Ptr, int Endpoint, int bInput, int Period,
	tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	
}

void *EHCI_InitIsoch(void *Ptr, int Endpoint)
{
	return NULL;
}
void *EHCI_InitControl(void *Ptr, int Endpoint)
{
	return NULL;
}
void *EHCI_InitBulk(void *Ptr, int Endpoint)
{
	return NULL;
}
void	EHCI_RemEndpoint(void *Ptr, void *Handle);
void	*EHCI_SendControl(void *Ptr, void *Dest, tUSBHostCb Cb, void *CbData,
	int isOutbound,
	const void *SetupData, size_t SetupLength,
	const void *OutData, size_t OutLength,
	void *InData, size_t InLength
	);
void	*EHCI_SendBulk(void *Ptr, void *Dest, tUSBHostCb Cb, void *CbData, int Dir, void *Data, size_t Length);


