/*
 */
#ifndef _MP_H
#define _MP_H

#define MPPTR_IDENT	('_'|('M'<<8)|('P'<<16)|('_'<<24))
#define MPTABLE_IDENT	('P'|('C'<<8)|('M'<<16)|('P'<<24))

typedef struct sMPInfo {
	Uint32	Sig;	// '_MP_'
	Uint32	MPConfig;
	Uint8	Length;
	Uint8	Version;
	Uint8	Checksum;
	Uint8	Features[5];	// 2-4 are unused
} tMPInfo;

typedef struct sMPTable {
	Uint32	Sig;
	Uint16	BaseTableLength;
	Uint8	SpecRev;
	Uint8	Checksum;
	
	char	OemID[8];
	char	ProductID[12];
	
	Uint32	OEMTablePtr;
	Uint16	OEMTableSize;
	Uint16	EntryCount;
	
	Uint32	LocalAPICMemMap;	//!< Address used to access the local APIC
	Uint16	ExtendedTableLen;
	Uint8	ExtendedTableChecksum;
	Uint8	Reserved;
} tMPTable;

typedef struct sMPTable_Proc {
	Uint8	Type;	// 0x00
	Uint8	APICID;
	Uint8	APICVer;
	Uint8	CPUFlags;	// bit 0: Disabled, bit 1: Boot Processor
	Uint32	CPUSignature;	// Stepping, Model, Family
	Uint32	FeatureFlags;
	Uint32	Reserved[2];
} tMPTable_Proc;

#endif
