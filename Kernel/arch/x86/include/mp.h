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

typedef union uMPTable_Ent {
	Uint8	Type;
	struct {
		Uint8	Type;
		Uint8	APICID;
		Uint8	APICVer;
		Uint8	CPUFlags;	// bit 0: Enabled, bit 1: Boot Processor
		Uint32	CPUSignature;	// Stepping, Model, Family
		Uint32	FeatureFlags;
		Uint32	Reserved[2];
	} __attribute__((packed))	Proc;	// 0x00
	struct {
		Uint8	Type;
		Uint8	ID;
		char	TypeString[6];
	} __attribute__((packed))	Bus;	// 0x01
	struct {
		Uint8	Type;
		Uint8	ID;
		Uint8	Version;
		Uint8	Flags;	// bit 0: Enabled
		Uint32	Addr;
	} __attribute__((packed))	IOAPIC;	// 0x02
	struct {
		Uint8	Type;
		Uint8	IntType;
		Uint16	Flags;	// 0,1: Polarity, 2,3: Trigger Mode
		Uint8	SourceBusID;
		Uint8	SourceBusIRQ;
		Uint8	DestAPICID;
		Uint8	DestAPICIRQ;
	} __attribute__((packed))	IOInt;
	struct {
		Uint8	Type;
		Uint8	IntType;
		Uint16	Flags;	// 0,1: Polarity, 2,3: Trigger Mode
		Uint8	SourceBusID;
		Uint8	SourceBusIRQ;
		Uint8	DestLocalAPICID;
		Uint8	DestLocalAPICIRQ;
	} __attribute__((packed))	LocalInt;
} __attribute__((packed)) tMPTable_Ent;

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
	
	tMPTable_Ent	Entries[];
} tMPTable;

typedef volatile struct {
	Uint8	Addr;
	Uint8	Resvd[3];
	Uint32	Resvd2[3];
	union {
		Uint8	Byte;
		Uint32	DWord;
	}	Value;
}	tIOAPIC;

#endif
