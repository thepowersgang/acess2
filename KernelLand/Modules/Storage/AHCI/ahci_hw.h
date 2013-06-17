/*
 * Acess2 Kernel - AHCI Driver
 * - By John Hodge (thePowersGang)
 *
 * ahci_hw.h
 * - Hardware Definitions
 */
#ifndef _AHCI__AHCI_HW_H_
#define _AHCI__AHCI_HW_H_

#define AHCI_CAP_S64A	(1 << 31)	// Supports 64-bit addressing
#define AHCI_CAP_SNCQ	(1 << 30)	// Supports Native Command Queuing
#define AHCI_CAP_SXS	(1 << 5)	// Support External SATA

#define AHCI_GHC_AE	(1 << 31)	// AHCI Enable
#define AHCI_GHC_MRSM	(1 << 2)	// MSI Revert to Single Message
#define AHCI_GHC_IE	(1 << 1)	// Interrupt Enable
#define ACHI_GHC_HR	(1 << 0)	// HBA Reset (Clears once complete)

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

