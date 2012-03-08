/*
 * Acess2 Kernel - AHCI Driver
 * - By John Hodge (thePowersGang)
 *
 * ahci.h
 * - Driver header
 */
#ifndef _AHCI__AHCI_H_
#define _AHCI__AHCI_H_

#define AHCI_CAP_S64A	(1 << 31)	// Supports 64-bit addressing
#define AHCI_CAP_SNCQ	(1 << 30)	// Supports Native Command Queuing
#define AHCI_CAP_SXS	(1 << 5)	// Support External SATA

struct sAHCI_MemSpace
{
	Uint32	CAP;	// Host Capabilities
	Uint32	GHC;	// Global Host Control;
	Uint16	IS;	// Interrupt Status
	Uint16	PI;	// Ports Implemented
	Uint32	VS;	// Version
	Uint32	CCC_CTL;	// Command Completion Coalsecing Control
	Uint16	CCC_PORTS;	// Command Completion Coalsecing Ports
	Uint32	EM_LOC;	// Enclosure Management Location
	Uint32	CAP2;	// Host Capabilities Extended
	Uint16	BOHC;	// BIOS/OS Handoff Control and Status
	
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
};

#endif

