/*
 * Acess2 Kernel - AHCI Driver
 * - By John Hodge (thePowersGang)
 *
 * ahci_hw.h
 * - Hardware Definitions
 */
#ifndef _AHCI__AHCI_HW_H_
#define _AHCI__AHCI_HW_H_

#include "sata.h"

#define AHCI_CAP_S64A	(1 << 31)	// Supports 64-bit addressing
#define AHCI_CAP_SNCQ	(1 << 30)	// Supports Native Command Queuing
#define AHCI_CAP_SXS	(1 << 5)	// Support External SATA
#define AHCI_CAP_NCS	(31 << 8)	// Number of command slots (mask)
#define AHCI_CAP_NCS_ofs	8	//                         (offset)

#define AHCI_GHC_AE	(1 << 31)	// AHCI Enable
#define AHCI_GHC_MRSM	(1 << 2)	// MSI Revert to Single Message
#define AHCI_GHC_IE	(1 << 1)	// Interrupt Enable
#define ACHI_GHC_HR	(1 << 0)	// HBA Reset (Clears once complete)

#define AHCI_PxIS_CPDS	(1 << 31)	// Cold Port Detect Status
#define AHCI_PxIS_TFES	(1 << 30)	// Task File Error Status
#define AHCI_PxIS_HBFS	(1 << 29)	// Host Bus Fatal error Status
#define AHCI_PxIS_HBDS	(1 << 28)	// Host Bus Data error Status
#define AHCI_PxIS_IFS	(1 << 27)	// Interface Fatal error Status
#define AHCI_PxIS_INFS	(1 << 26)	// Interface Non-Fatal error status
#define AHCI_PxIS_OFS	(1 << 24)	// OverFlow Status
#define AHCI_PxIS_IPMS	(1 << 23)	// Incorrect Port Multipier Status
#define AHCI_PxIS_PRCS	(1 << 22)	// PhyRdy Change Status
#define AHCI_PxIS_DMPS	(1 << 7)	// Device Mechanical Presence Status
#define AHCI_PxIS_PCS	(1 << 6)	// Port Connect change Status
#define AHCI_PxIS_DPS	(1 << 5)	// Descriptor Processed
#define AHCI_PxIS_UFI	(1 << 4)	// Unknown FIX Interrupt
#define AHCI_PxIS_SDBS	(1 << 3)	// Set Device Bits Interrupt
#define AHCI_PxIS_DSS	(1 << 2)	// DMA Setup FIS Interrupt
#define AHCI_PxIS_PSS	(1 << 1)	// PIO Setup FIX Interrupt
#define AHCI_PxIS_DHRS	(1 << 0)	// Device to Host Register FIS Interrupt

#define AHCI_PxCMD_ICC	(15 << 28)	// Interface Communication Control (mask)
#define AHCI_PxCMD_ASP	(1 << 27)	// Agressive Slumber / Partial
#define AHCI_PxCMD_ALPE	(1 << 26)	// Agressive Link Power Management Enable
#define AHCI_PxCMD_DLAE	(1 << 25)	// Drive LED on ATAPI Enable
#define AHCI_PxCMD_ATAPI	(1 << 24)	// Device is ATAPI
#define AHCI_PxCMD_APSTE	(1 << 23)	// Automatic Partial to Slumber Transitions Enabled
#define AHCI_PxCMD_FBSCP	(1 << 22)	// FIS-based Switching Capable Port
#define AHCI_PxCMD_ESP	(1 << 21)	// External SATA Port
#define AHCI_PxCMD_CPD	(1 << 20)	// Cold Presence Detection
#define AHCI_PxCMD_MPSP	(1 << 19)	// Mechanical Presence Switch attached to Port
#define AHCI_PxCMD_HPCP	(1 << 18)	// Hot Plut Capable Port
#define AHCI_PxCMD_PMA	(1 << 17)	// Port Multiplier Attached
#define AHCI_PxCMD_CPS	(1 << 16)	// Cold Presence State
#define AHCI_PxCMD_CR	(1 << 15)	// Command List Running
#define AHCI_PxCMD_FR	(1 << 14)	// FIS Receive Running
#define AHCI_PxCMD_MPSS	(1 << 13)	// Mechanical Presence Switch State
#define AHCI_PxCMD_CCS	(31 << 8)	// Current Command Slot (mask)
#define AHCI_PxCMD_FRE	(1 << 4)	// FIS Receive Enable
#define AHCI_PxCMD_CLO	(1 << 3)	// Command List Override
#define AHCI_PxCMD_POD	(1 << 2)	// Power On Device
#define AHCI_PxCMD_SUD	(1 << 1)	// Spin-Up Device
#define AHCI_PxCMD_ST	(1 << 0)	// Start

#define AHCI_PxTFD_ERR	(255 << 8)
#define AHCI_PxTFD_STS	(255 << 0)	// Status (latest copy of task file status register)
#define AHCI_PxTFD_STS_BSY	(1 << 7)	// Interface is busy
#define AHCI_PxTFD_STS_DRQ	(1 << 3)	// Data transfer requested
#define AHCI_PxTFD_STS_ERR	(1 << 0)	// Error during transfer

#define AHCI_PxSSTS_IPM	(15 << 8)	// Interface Power Management (0=NP,1=Active,2=Partial,6=Slumber)
#define AHCI_PxSSTS_IPM_ofs	8
#define AHCI_PxSSTS_SPD	(15 << 4)	// Current Interface Speed (0=NP,Generation n)
#define AHCI_PxSSTS_SPD_ofs	4
#define AHCI_PxSSTS_DET	(15 << 0)	// Device Detection (0: None, 1: Present but no PHY yet, 3: Present and PHY, 4: offline)
#define AHCI_PxSSTS_DET_ofs	0

typedef volatile struct sAHCI_MemSpace	tAHCI_MemSpace;

struct sAHCI_MemSpace
{
	Uint32	CAP;	// Host Capabilities
	Uint32	GHC;	// Global Host Control;
	Uint32	IS;	// Interrupt Status
	Uint32	PI;	// Ports Implemented
	Uint32	VS;	// Version
	Uint32	CCC_CTL;	// Command Completion Coalsecing Control
	Uint32	CCC_PORTS;	// Command Completion Coalsecing Ports
	Uint32	EM_LOC;	// Enclosure Management Location
	Uint32	EM_CTL;	// Enclosure management control
	Uint32	CAP2;	// Host Capabilities Extended
	Uint32	BOHC;	// BIOS/OS Handoff Control and Status
	
	Uint16	_padding[(0x100-0x2C)/2];

	struct s_port
	{
		Uint32	PxCLB;	// Command List Base Address
		Uint32	PxCLBU;	// (High of above)
		Uint32	PxFB;	// FIS Base Address
		Uint32	PxFBU;	// (high of above)
		Uint32	PxIS;	// Interrupt Status
		Uint32	PxIE;	// Interrupt Enable
		Uint32	PxCMD;	// Command and Status
		Uint32	_resvd;
		Uint32	PxTFD;	// Task File Data
		Uint32	PxSIG;	// Signature
		Uint32	PxSSTS;	// Serial ATA Status
		Uint32	PxSCTL;	// Serial ATA Control
		Uint32	PxSERR;	// Serial ATA Error
		Uint32	PxSACT;	// Serial ATA Active
		Uint32	PxCI;	// Command Issue
		Uint32	PxSNTF;	// Serial ATA Notification
		Uint32	PxFBS;	// FIS-based Switching Control
		Uint32	_resvd2[(0x70-0x44)/4];
		Uint32	PxVS[4];
	}	Ports[32];
} PACKED;

struct sAHCI_RcvdFIS
{
	struct sSATA_FIS_DMASetup	DSFIS;
	Uint32	_pad1[1];
	struct sSATA_FIS_PIOSetup	PSFIS;
	Uint32	_pad2[3];
	struct sSATA_FIS_D2HRegister	RFIS;
	Uint32	_pad3[1];
	struct sSATA_FIS_SDB	SDBFIS;
	Uint32	UFIS[64/4];
	Uint32	_redvd[(0x100 - 0xA0) / 4];
} PACKED;

struct sAHCI_CmdHdr
{
	Uint16	Flags;
	Uint16	PRDTL;
	Uint32	PRDBC;
	Uint32	CTBA;	// 128-byte alignment
	Uint32	CTBAU;
	Uint32	resdv[4];
} PACKED;

struct sAHCI_CmdEnt
{
	Uint32	DBA;	// Data base address
	Uint32	DBAU;	// (upper)
	Uint32	resvd;
	Uint32	DBC;	// Data byte count (31: IOC, 21:0 count) 0=1, 1=2, ...
} PACKED;

#endif

