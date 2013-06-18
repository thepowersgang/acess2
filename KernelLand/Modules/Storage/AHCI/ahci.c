/*
 * Acess2 Kernel - AHCI Driver
 * - By John Hodge (thePowersGang)
 *
 * ahci.c
 * - Driver core
 */
#define DEBUG	1
#define VERSION	0x0001
#include <acess.h>
#include <timers.h>
#include <modules.h>
#include <drv_pci.h>
#include "ahci.h"

// === CONSTANTS ===
#define MAX_CONTROLLERS	4

// === PROTOTYPES ===
 int	AHCI_Install(char **Arguments);
 int	AHCI_Cleanup(void);
// - Hardware init
 int	AHCI_InitSys(tAHCI_Ctrlr *Ctrlr);

// === GLOABLS ===
MODULE_DEFINE(0, VERSION, AHCI, AHCI_Install, AHCI_Cleanup, "LVM", NULL);
tAHCI_Ctrlr	gaAHCI_Controllers[MAX_CONTROLLERS];

// === CODE ====
int AHCI_Install(char **Arguments)
{
	 int	rv;
	LOG("offsetof(struct sAHCI_MemSpace, Ports) = %x", offsetof(struct sAHCI_MemSpace, Ports));
	ASSERT( offsetof(struct sAHCI_MemSpace, Ports) == 0x100 );

	// 0106XX = Standard, 010400 = RAID
	 int	i = 0;
	 int	id = -1;
	while( (id = PCI_GetDeviceByClass(0x010601, 0xFFFFFF, id)) >= 0 && i < MAX_CONTROLLERS )
	{
		tAHCI_Ctrlr *ctrlr = &gaAHCI_Controllers[i];
		ctrlr->PMemBase = PCI_GetBAR(id, 5);
		// 
		if( !ctrlr->PMemBase )
		{
			// ctrlr->PMemBase = PCI_AllocateBAR(id, 5);
			// TODO: Allocate space
			continue ;
		}
		// - IO Address?
		if( (ctrlr->PMemBase & 0x1FFF) ) {
			Log_Warning("AHCI", "Controller %i [PCI %i] is invalid (BAR5=%P)",
				i, id, ctrlr->PMemBase);
			continue;
		}
		
		ctrlr->IRQ = PCI_GetIRQ(id);

		// Prepare MMIO (two pages according to the spec)
		ctrlr->MMIO = (void*)MM_MapHWPages(ctrlr->PMemBase, 2);

		LOG("%i [%i]: %P/IRQ%i mapped to %p",
			i, id, ctrlr->PMemBase, ctrlr->IRQ, ctrlr->MMIO);

		LOG(" CAP = %x, PI = %x, VS = %x",
			ctrlr->MMIO->CAP, ctrlr->MMIO->PI, ctrlr->MMIO->VS);

		if( (rv = AHCI_InitSys(ctrlr)) ) {
			// Clean up controller's allocations
			// TODO: Should an init error cause module unload?
			return rv;
		}	

		i ++;
	}
	if( id >= 0 ) {
		Log_Notice("AHCI", "Only up to %i controllers are supported", MAX_CONTROLLERS);
	}
	
	return 0;
}

int AHCI_Cleanup(void)
{
	return 0;
}

static inline void *AHCI_AllocPage(tAHCI_Ctrlr *Ctrlr, const char *AllocName)
{
	void	*ret;
	#if PHYS_BITS > 32
	if( Ctrlr->Supports64Bit )
		ret = (void*)MM_AllocDMA(1, 64, NULL);
	else
	#endif
		ret = (void*)MM_AllocDMA(1, 32, NULL);
	if( !ret ) {
		Log_Error("AHCI", "Unable to allocate page for '%s'", AllocName);
		return NULL;
	}
	return ret;
}

int AHCI_InitSys(tAHCI_Ctrlr *Ctrlr)
{
	// 1. Set GHC.AE
	Ctrlr->MMIO->GHC |= AHCI_GHC_AE;
	// 2. Enumerate ports using PI
	tTime	basetime = now();
	for( int i = 0; i < 32; i ++ )
	{
		if( !(Ctrlr->MMIO->PI & (1 << i)) )
			continue ;
		
		Ctrlr->PortCount ++;
		
		volatile struct s_port	*port = &Ctrlr->MMIO->Ports[i];
		// 3. (for each port) Ensure that port is not running
		//  - if PxCMD.(ST|CR|FRE|FR) all are clear, port is idle
		if( (port->PxCMD & (AHCI_PxCMD_ST|AHCI_PxCMD_CR|AHCI_PxCMD_FRE|AHCI_PxCMD_FR)) == 0 )
			continue ;
		//  - Idle is set by clearing PxCMD.ST and waiting for .CR to clear (timeout 500ms)
		//  - AND .FRE = 0, checking .FR (timeout 500ms again)
		port->PxCMD = 0;
		basetime = now();
		//  > On timeout, port/HBA reset
	}
	Ctrlr->Ports = malloc( Ctrlr->PortCount * sizeof(*Ctrlr->Ports) );
	if( !Ctrlr->Ports ) {
		return MODULE_ERR_MALLOC;
	}
	// - Process timeouts after all ports have been poked, saves time
	for( int i = 0, idx = 0; i < 32; i ++ )
	{
		if( !(Ctrlr->MMIO->PI & (1 << i)) )
			continue ;
		volatile struct s_port	*port = &Ctrlr->MMIO->Ports[i];
		Ctrlr->Ports[idx].Idx = i;
		Ctrlr->Ports[idx].MMIO = port;
		idx ++;
	
		while( (port->PxCMD & (AHCI_PxCMD_CR|AHCI_PxCMD_FR)) && now()-basetime < 500 )
			Time_Delay(10);
		
		if( (port->PxCMD & (AHCI_PxCMD_CR|AHCI_PxCMD_FR)) )
		{
			Log_Error("AHCI", "Port %i did not return to idle", i);
		}
	}
	// 4. Read CAP.NCS to get number of command slots
	Ctrlr->NCS = (Ctrlr->MMIO->CAP & AHCI_CAP_NCS) >> AHCI_CAP_NCS_ofs;
	// 5. Allocate PxCLB and PxFB for each port (setting PxCMD.FRE once done)
	struct sAHCI_RcvdFIS	*fis_vpage = NULL;
	struct sAHCI_CmdHdr	*cl_vpage = NULL;
	for( int i = 0; i < Ctrlr->PortCount; i ++ )
	{
		// CLB First (1 KB alignemnt)
		if( ((tVAddr)cl_vpage & 0xFFF) == 0 ) {
			cl_vpage = AHCI_AllocPage(Ctrlr, "CLB");
			if( !cl_vpage )
				return MODULE_ERR_MALLOC;
		}
		Ctrlr->Ports[i].CmdList = cl_vpage;
		cl_vpage += 1024/sizeof(*cl_vpage);

		tPAddr	cl_paddr = MM_GetPhysAddr(Ctrlr->Ports[i].CmdList);
		Ctrlr->Ports[i].MMIO->PxCLB  = cl_paddr;
		#if PHYS_BITS > 32
		Ctrlr->Ports[i].MMIO->PxCLBU = cl_paddr >> 32;
		#endif

		// Received FIS Area
		// - If there is space for the FIS in the end of the 1K block, use it
		if( Ctrlr->NCS <= (1024-256)/32 )
		{
			Ctrlr->Ports[i].RcvdFIS = (void*)(cl_vpage - 256/32);
		}
		else
		{
			if( ((tVAddr)fis_vpage & 0xFFF) == 0 ) {
				fis_vpage = AHCI_AllocPage(Ctrlr, "FIS");
				if( !fis_vpage )
					return MODULE_ERR_MALLOC;
			}
			Ctrlr->Ports[i].RcvdFIS = fis_vpage;
			fis_vpage ++;
		}
		tPAddr	fis_paddr = MM_GetPhysAddr(Ctrlr->Ports[i].RcvdFIS);
		Ctrlr->Ports[i].MMIO->PxFB  = fis_paddr;
		#if PHYS_BITS > 32
		Ctrlr->Ports[i].MMIO->PxFBU = fis_paddr >> 32;
		#endif
		
		LOG("Port #%i: CLB=%p/%P, FB=%p/%P", i,
			Ctrlr->Ports[i].CmdList, cl_paddr,
			Ctrlr->Ports[i].RcvdFIS, fis_paddr);
	}
	// 6. Clear PxSERR with 1 to each implimented bit
	// > Clear PxIS then IS.IPS before setting PxIE/GHC.IE
	// 7. Set PxIE with desired interrupts and set GHC.IE
	
	// Detect present ports using:
	// > PxTFD.STS.BSY = 0, PxTFD.STS.DRQ = 0, and PxSSTS.DET = 3
	for( int i = 0; i < Ctrlr->PortCount; i ++ )
	{
		if( Ctrlr->Ports[i].MMIO->PxTFD & (AHCI_PxTFD_STS_BSY|AHCI_PxTFD_STS_DRQ) )
			continue ;
		if( (Ctrlr->Ports[i].MMIO->PxSSTS & AHCI_PxSSTS_DET) != 3 << AHCI_PxSSTS_DET_ofs )
			continue ;
		
		LOG("Port #%i: Connection detected", i);
		Ctrlr->Ports[i].bPresent = 1;
		// TODO: Query volumes connected
	}
	return 0;
}

