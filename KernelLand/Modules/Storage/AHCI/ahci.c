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
void	AHCI_QueryDevice(tAHCI_Ctrlr *Ctrlr, int PortNum);
void	AHCI_IRQHandler(int UNUSED(IRQ), void *Data);
void	AHCI_int_IRQHandlerPort(tAHCI_Port *Port);
// - LVM Interface
int	AHCI_ReadSectors(void *Ptr, Uint64 Address, size_t Count, void *Buffer);
int	AHCI_WriteSectors(void *Ptr, Uint64 Address, size_t Count, const void *Buffer);
// - Low Level
 int	AHCI_SendLBA28Cmd(tAHCI_Port *Port, int bWrite,
		Uint8 Dev, Uint8 Sectors, Uint64 LBA, Uint8 Cmd, size_t Size, void *Data
		);
 int	AHCI_SendLBA48Cmd(tAHCI_Port *Port, int bWrite,
		Uint8 Dev, Uint8 Sectors, Uint64 LBA, Uint8 Cmd, size_t Size, void *Data
		);
 int	AHCI_DoFIS(tAHCI_Port *Port, int bWrite,
		size_t CmdSize, const void *CmdData, size_t PktSize, const void *PktData,
		size_t OutSize, void *OutData
		);
 int	AHCI_WaitForInterrupt(tAHCI_Port *Port, unsigned int Timeout);

// === GLOABLS ===
MODULE_DEFINE(0, VERSION, AHCI, AHCI_Install, AHCI_Cleanup, "LVM", NULL);
tAHCI_Ctrlr	gaAHCI_Controllers[MAX_CONTROLLERS];
tLVM_VolType	gAHCI_VolumeType = {
	.Name = "AHCI",
	.Read = AHCI_ReadSectors,
	.Write = AHCI_WriteSectors
	};

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
		ctrlr->ID = i;
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
		IRQ_AddHandler(ctrlr->IRQ, AHCI_IRQHandler, ctrlr);

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

static inline void AHCI_int_SetAddr(tAHCI_Ctrlr *Ctrlr, volatile Uint32 *Addr, tPAddr PAddr)
{
	Addr[0] = PAddr;
	#if PHYS_BITS > 32
	if(Ctrlr->Supports64Bit)
		Addr[1] = PAddr >> 32;
	else if( PAddr >> 32 )
		Log_Notice("AHCI", "Bug: 64-bit address used with 32-bit only controller");
	else
	#endif
		Addr[1] = 0;
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
		Ctrlr->Ports[idx].Ctrlr = Ctrlr;
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
	struct sAHCI_CmdTable	*ct_vpage = NULL;
	for( int i = 0; i < Ctrlr->PortCount; i ++ )
	{
		tAHCI_Port	*port = &Ctrlr->Ports[i];
		// CLB First (1 KB alignemnt)
		if( ((tVAddr)cl_vpage & 0xFFF) == 0 ) {
			cl_vpage = AHCI_AllocPage(Ctrlr, "CLB");
			if( !cl_vpage )
				return MODULE_ERR_MALLOC;
		}
		port->CmdList = cl_vpage;
		cl_vpage += 1024/sizeof(*cl_vpage);

		tPAddr	cl_paddr = MM_GetPhysAddr(port->CmdList);
		AHCI_int_SetAddr(Ctrlr, &port->MMIO->PxCLB, cl_paddr);

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
		AHCI_int_SetAddr(Ctrlr, &port->MMIO->PxFB, fis_paddr);
	
		// Command tables
		for( int j = 0; j < Ctrlr->NCS; j ++ )
		{
			if( ((tVAddr)ct_vpage & 0xFFF) == 0 ) {
				ct_vpage = AHCI_AllocPage(Ctrlr, "CmdTab");
				if( !ct_vpage )
					return MODULE_ERR_MALLOC;
			}
			port->CommandTables[j] = ct_vpage;
			port->CmdList[j].Flags = 0;
			port->CmdList[j].PRDTL = 0;
			port->CmdList[j].PRDBC = 0;
			AHCI_int_SetAddr(Ctrlr, &port->CmdList[j].CTBA, MM_GetPhysAddr(ct_vpage));
			ct_vpage ++;
		}
		
		LOG("Port #%i: CLB=%p/%P, FB=%p/%P", i,
			Ctrlr->Ports[i].CmdList, cl_paddr,
			Ctrlr->Ports[i].RcvdFIS, fis_paddr);
	}
	
	// 6. Clear PxSERR with 1 to each implimented bit
	// > Clear PxIS then IS.IPS before setting PxIE/GHC.IE
	// 7. Set PxIE with desired interrupts and set GHC.IE
	for( int i = 0; i < Ctrlr->PortCount; i ++ )
	{
		tAHCI_Port	*port = &Ctrlr->Ports[i];
		
		port->MMIO->PxSERR = 0x3FF783;
		port->MMIO->PxIS = -1;
		port->MMIO->PxIE = AHCI_PxIS_CPDS|AHCI_PxIS_DSS|AHCI_PxIS_PSS|AHCI_PxIS_DHRS;
	}
	Ctrlr->MMIO->IS = -1;
	Ctrlr->MMIO->GHC |= AHCI_GHC_IE;

	// Start command engine on all implimented ports
	for( int i = 0; i < Ctrlr->PortCount; i ++ )
	{
		tAHCI_Port	*port = &Ctrlr->Ports[i];
		port->MMIO->PxCMD |= AHCI_PxCMD_ST|AHCI_PxCMD_FRE;
	}
	
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
		AHCI_QueryDevice(Ctrlr, i);
	}
	return 0;
}

static inline void _flipChars(char *buffer, size_t pairs)
{
	for( int i = 0; i < pairs; i ++ )
	{
		char tmp = buffer[i*2];
		buffer[i*2] = buffer[i*2+1];
		buffer[i*2+1] = tmp;
	}
}

void AHCI_QueryDevice(tAHCI_Ctrlr *Ctrlr, int PortNum)
{
	tAHCI_Port	*const Port = &Ctrlr->Ports[PortNum];
	tATA_Identify	data;	

	AHCI_SendLBA28Cmd(Port, false, 0, 0, 0, ATA_CMD_IDENTIFY_DEVICE, sizeof(data), &data);
	if( AHCI_WaitForInterrupt(Port, 1000) ) {
		Log_Error("AHCI", "Port %i:%i ATA IDENTIFY_DEVICE timed out", Ctrlr->ID, PortNum);
		return ;
	}
	// TODO: Check status from command
	// TODO: on error, mark device as bad and return

	_flipChars(data.SerialNum, 20/2);
	_flipChars(data.FirmwareVer, 8/2);
	_flipChars(data.ModelNumber, 40/2);
	LOG("data.SerialNum   = '%.20s'", data.SerialNum);
	LOG("data.FirmwareVer = '%.8s'", data.FirmwareVer);
	LOG("data.ModelNumber = '%.40s'", data.ModelNumber);

	if( data.Sectors48 != 0 ) {
		// Use LBA48 size
		LOG("Size[48] = 0x%X", (Uint64)data.Sectors48);
		Port->SectorCount = data.Sectors48;
	}
	else {
		// Use LBA28 size
		LOG("Size[28] = 0x%x", data.Sectors28);
		Port->SectorCount = data.Sectors28;
	}
	
	// Create LVM name
	#if AHCI_VOLNAME_SERIAL
	char lvmname[4+1+20+1];
	strcpy(lvmname, "ahci:");
	memcpy(lvmname+5, data.SerialNum, 20);
	for(int i = 20+5; i-- && lvmname[i] == ' '; )
		lvmname[i] = '\0';
	#else
	char lvmname[5+3+4+1];
	snprintf(lvmname, sizeof(lvmname), "ahci:%i-%i", Ctrlr->ID, PortNum);
	#endif
	
	// Register with LVM
	Port->LVMHandle = LVM_AddVolume(&gAHCI_VolumeType, lvmname, Port, 512, Port->SectorCount);
}

void AHCI_IRQHandler(int UNUSED(IRQ), void *Data)
{
	tAHCI_Ctrlr	*Ctrlr = Data;
	tAHCI_Port	*port = &Ctrlr->Ports[0];
	
	Uint32	IS = Ctrlr->MMIO->IS;
	Uint32	PI = Ctrlr->MMIO->PI;
	LOG("Ctrlr->MMIO->IS = %x", IS);

	for( int i = 0; i < 32; i ++ )
	{
		if( !(PI & (1 << i)) )
			continue ;
		if( IS & (1 << i) )
		{
			AHCI_int_IRQHandlerPort(port);
		}
		port ++;
	}	

	Ctrlr->MMIO->IS = IS;
}

void AHCI_int_IRQHandlerPort(tAHCI_Port *Port)
{
	Uint32	PxIS = Port->MMIO->PxIS;
	LOG("port->MMIO->PxIS = %x", PxIS);
	Port->LastIS |= PxIS;
	if( PxIS & AHCI_PxIS_CPDS ) {
		// Cold port detect change detected
		// TODO: Handle removal of a device by poking LVM.
	}

	if( PxIS & AHCI_PxIS_DHRS ) {
		LOG("Port->RcvdFIS->RFIS = {");
		LOG(".Status = 0x%02x", Port->RcvdFIS->RFIS.Status);
		LOG(".Error = 0x%02x", Port->RcvdFIS->RFIS.Error);
		LOG("}");
	}	

	// Get bitfield of completed commands (Issued but no activity)
	Uint32	done_commands = Port->IssuedCommands ^ Port->MMIO->PxSACT;
	LOG("done_commands = %x", done_commands);
	for( int i = 0; i < 32; i ++ )
	{
		if( !(done_commands & (1 << i)) )
			continue;
		ASSERT( Port->IssuedCommands & (1 << i) );
		// If complete, post a SHORTWAIT
		if( Port->CommandThreads[i] ) {
			LOG("%i - Poking thread %p", i, Port->CommandThreads[i]);
			Threads_PostEvent(Port->CommandThreads[i], THREAD_EVENT_SHORTWAIT);
			Port->CommandThreads[i] = NULL;
		}
		Port->IssuedCommands &= ~(1 << i);
	}
	Port->MMIO->PxIS = PxIS;
}

// - LVM Interface
int AHCI_ReadSectors(void *Ptr, Uint64 Address, size_t Count, void *Buffer)
{
	tAHCI_Port	*Port = Ptr;

	memset(Buffer, 0xFF, Count*512);
	
	ASSERT(Count <= 8096/512);
	if( (Address+Count) < (1 << 24) )
		AHCI_SendLBA28Cmd(Port, 0, 0, Count, Address, ATA_CMD_READDMA28, Count*512, Buffer);
	else
		AHCI_SendLBA48Cmd(Port, 0, 0, Count, Address, ATA_CMD_READDMA48, Count*512, Buffer);
	if( AHCI_WaitForInterrupt(Port, 1000) ) {
		Log_Notice("AHCI", "Timeout reading from disk");
		return 0;
	}
	if( Port->RcvdFIS->RFIS.Status & ATA_STATUS_ERR )
	{
		LOG("Error detected = 0x%02x", Port->RcvdFIS->RFIS.Error);
		return 0;
	}
	//Debug_HexDump("AHCI_ReadSectors", Buffer, Count*512);
	// TODO: Check status from command
	return Count;
}

int AHCI_WriteSectors(void *Ptr, Uint64 Address, size_t Count, const void *Buffer)
{
	return 0;
}

// -- Internals
int AHCI_int_GetCommandSlot(tAHCI_Port *Port)
{
	Mutex_Acquire(&Port->lCommandSlots);
	Uint32	PxCI = Port->MMIO->PxCI;
	Uint32	PxSACT = Port->MMIO->PxSACT;
	for( int i = 0; i < Port->Ctrlr->NCS; i ++ )
	{
		if( PxCI & (1 << i) )
			continue ;
		if( PxSACT & (1 << i) )
			continue ;
		LOG("Allocated slot %i", i);
		Port->IssuedCommands |= (1 << i);
		return i;
	}
	return -1;
}
void AHCI_int_StartCommand(tAHCI_Port *Port, int Slot)
{
	Port->MMIO->PxCI = 1 << Slot;	
	Mutex_Release(&Port->lCommandSlots);
}
void AHCI_int_CancelCommand(tAHCI_Port *Port, int Slot)
{
	// Release command
	Port->IssuedCommands &= ~(1 << Slot);
	Mutex_Release(&Port->lCommandSlots);
}

// -- Low Level
int AHCI_SendLBA48Cmd(tAHCI_Port *Port, int bWrite,
	Uint8 Dev, Uint8 Sectors, Uint64 LBA, Uint8 Cmd, size_t Size, void *Data)
{
	struct sSATA_FIS_H2DRegister	regs;
	
	regs.Type = SATA_FIS_H2DRegister;
	regs.Flags = 0x80;	// [7]: Update to command register
	regs.Command = Cmd;
	regs.Features = 0;
	
	regs.SectorNum = LBA;
	regs.CylLow = LBA >> 8;
	regs.CylHigh = LBA >> 16;
	regs.Dev_Head = 0x40|Dev;	// TODO: Need others?
	
	regs.SectorNumExp = LBA >> 24;
	regs.CylLowExp = LBA >> 32;
	regs.CylHighExp = LBA >> 40;
	regs.FeaturesExp = 0;
	
	regs.SectorCount = Sectors;
	regs.SectorCountExp = 0;
	regs.Control = 0;

	LOG("Sending command %02x with %p+0x%x", Cmd, Data, Size);
	AHCI_DoFIS(Port, bWrite, sizeof(regs), &regs, 0, NULL, Size, Data);

	return 0;
}

int AHCI_SendLBA28Cmd(tAHCI_Port *Port, int bWrite,
	Uint8 Dev, Uint8 Sectors, Uint64 LBA, Uint8 Cmd, size_t Size, void *Data)
{
	struct sSATA_FIS_H2DRegister	regs;

	ASSERT(LBA < (1 << 24));
	
	regs.Type = SATA_FIS_H2DRegister;
	regs.Flags = 0x80;	// [7]: Update to command register
	regs.Command = Cmd;
	regs.Features = 0;
	
	regs.SectorNum = LBA;
	regs.CylLow = LBA >> 8;
	regs.CylHigh = LBA >> 16;
	regs.Dev_Head = 0x40|Dev|(LBA >> 24);
	
	regs.SectorNumExp = 0;
	regs.CylLowExp = 0;
	regs.CylHighExp = 0;
	regs.FeaturesExp = 0;
	
	regs.SectorCount = Sectors;
	regs.SectorCountExp = 0;
	regs.Control = 0;

	LOG("Sending command %02x with %p+0x%x", Cmd, Data, Size);
	AHCI_DoFIS(Port, bWrite, sizeof(regs), &regs, 0, NULL, Size, Data);

	return 0;
}

int AHCI_ReadFIS(tAHCI_Port *Port,
	size_t CmdSize, const void *CmdData, size_t PktSize, const void *PktData,
	size_t InSize, void *InData)
{
	return AHCI_DoFIS(Port, 0, CmdSize, CmdData, PktSize, PktData, InSize, InData);
}
int AHCI_WriteFIS(tAHCI_Port *Port,
	size_t CmdSize, const void *CmdData, size_t PktSize, const void *PktData,
	size_t OutSize, const void *OutData)
{
	return AHCI_DoFIS(Port, 1, CmdSize, CmdData, PktSize, PktData, OutSize, (void*)OutData);
}

int AHCI_DoFIS(tAHCI_Port *Port, int bWrite,
	size_t CmdSize, const void *CmdData, size_t PktSize, const void *PktData,
	size_t OutSize, void *OutData)
{
	// 1. Obtain a command slot
	int slot = AHCI_int_GetCommandSlot(Port);
	if( slot < 0 ) {
		return -1;
	}
	ASSERT(slot < 32);
	struct sAHCI_CmdTable	*cmdt = Port->CommandTables[slot];

	// 2. Fill commands
	if( CmdSize > 64 ) {
		Log_Error("AHCI", "_DoFIS: Command FIS size %i > 64", CmdSize);
		goto error;
	}
	memcpy(cmdt->CFIS, CmdData, CmdSize);
	if(PktSize > 16) {
		Log_Error("AHCI", "_DoFIS: ATAPI packet size %i > 64", PktSize);
		goto error;
	}
	memcpy(cmdt->ACMD, PktData, PktSize);
	
	// 3. Set pointers
	size_t	ofs = 0;
	 int	prdtl = 0;
	while( ofs < OutSize )
	{
		tPAddr	phys = MM_GetPhysAddr( (char*)OutData + ofs );
		ASSERT( !(phys & 3) );
		// TODO: must be 4 byte aligned, and handle 64-bit addressing
		size_t	len = MIN(OutSize - ofs, PAGE_SIZE - (phys % PAGE_SIZE));
		ASSERT( !(len & 1) );
		ASSERT( len < 4*1024*1024 );
		LOG("PRDTL[%i] = %P+%i", prdtl, phys, len);
		// TODO: count must be even.
		AHCI_int_SetAddr(Port->Ctrlr, &cmdt->PRDT[prdtl].DBA, phys);
		cmdt->PRDT[prdtl].DBC = len-1;
		prdtl ++;
		ofs += len;
	}
	ASSERT(ofs == OutSize);
	cmdt->PRDT[prdtl-1].DBC |= (1<<31);	// Set IOC
	
	// TODO: Port multipliers
	Port->CmdList[slot].PRDTL = prdtl;
	Port->CmdList[slot].Flags = (bWrite << 6) | (CmdSize / 4);
	
	// Prepare interrupt
	Port->CommandThreads[slot] = Proc_GetCurThread();
	Threads_ClearEvent(THREAD_EVENT_SHORTWAIT);

	// 4. Dispatch
	AHCI_int_StartCommand(Port, slot);

	return 0;
error:
	AHCI_int_CancelCommand(Port, slot);
	return -1;
}

int AHCI_WaitForInterrupt(tAHCI_Port *Port, unsigned int Timeout)
{
	// Set up a timeout callback
	Threads_ClearEvent(THREAD_EVENT_TIMER);
	tTimer *timeout = Time_AllocateTimer(NULL, NULL);
	Time_ScheduleTimer(timeout, Timeout);
	
	// Wait until an interrupt arrives or the timeout fires
	Uint32 ev = Threads_WaitEvents(THREAD_EVENT_SHORTWAIT|THREAD_EVENT_TIMER);
	Time_FreeTimer(timeout);

	if( ev & THREAD_EVENT_TIMER ) {
		Log_Notice("AHCI", "Timeout of %i ms exceeded", Timeout);
		return 1;
	}
	return 0;
}

