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

// === CONSTANTS ===
#define MAX_CONTROLLERS	4

// === PROTOTYPES ===
 int	OHCI_Initialise(char **Arguments);
void	OHCI_Cleanup(void);

void	*OHCI_DataIN(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
void	*OHCI_DataOUT(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData,  void *Buf, size_t Length);
void	*OHCI_SendSetup(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, USB_OHCI, OHCI_Initialise, OHCI_Cleanup, "USB_Core", NULL);
tUSBHostDef	gOHCI_HostDef = {
	.SendIN = OHCI_DataIN,
	.SendOUT = OHCI_DataOUT,
	.SendSETUP = OHCI_DataSETUP,
	};

// === CODE ===
int OHCI_Initialise(char **Arguments)
{
	 int	id;
	 int	card_num = 0;
	
	while( (id = PCI_GetDeviceByClass(0x0C0310, 0xFFFFFF)) != -1 && card_num < MAX_CONTROLLERS )
	{
		Uint32	base_addr;
		
		base_addr = PCI_GetBAR(id, 0);	// Offset 0x10
	}

	
	// TODO: Initialise
	return 0;
}

void OHCI_Cleanup(void)
{
	// TODO: Cleanup for unload
}

void *OHCI_DataIN(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length)
{
	return NULL;
}
void *OHCI_DataOUT(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData,  void *Buf, size_t Length);
{
	return NULL;
}
void *OHCI_SendSetup(void *Ptr, int Dest, int DataTgl, tUSBHostCb Cb, void *CbData, void *Buf, size_t Length);
{
	return NULL;
}
