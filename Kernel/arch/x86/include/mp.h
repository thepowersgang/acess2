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
	Uint32	Addr;
	Uint32	Resvd1[3];
	union {
		Uint8	Byte;
		Uint16	Word;
		Uint32	DWord;
	}	Value;
	Uint32	Resvd2[3];
}	tIOAPIC;

typedef struct {
	Uint32	Val;
	Uint32	Padding[3];
}	tReg;

typedef volatile struct {
	tReg	_unused1[2];
	tReg	ID;
	tReg	Version;
	tReg	_unused2[4];
	tReg	TPR;	// Task Priority Register
	tReg	APR;	// Arbitration Priority Register (RO)
	tReg	PPR;	// Processor Priority Register (RO)
	tReg	EOI;	// EOI Register (Write Only)
	tReg	_unused3[1];
	tReg	LogDest;	// Logical Destination Register
	tReg	DestFmt;	// Destination Format Register (0-27: RO, 28-31: RW)
	tReg	SIV;	// Spurious Interrupt Vector Register (0-8: RW, 9-31: RO)
	tReg	ISR[8];	// In-Service Register - Total 256 Bits (RO)
	tReg	TMR[8];	// Trigger Mode Register - Total 256 Bits (RO)
	tReg	IRR[8];	// Interrupt Request Register - Total 256 Bits (RO)
	tReg	ErrorStatus;	// Error Status Register (RO)
	tReg	_unused4[6];
	tReg	LVTCMI;	// LVT CMI Registers
	// 0x300
	tReg	ICR[2];	// Interrupt Command Register (RW)
	// LVT Registers (Controls Local Vector Table)
	// Structure:
	// 0-7:   Vector - IDT Vector for the interrupt
	// 12:    Delivery Status (0: Idle, 1: Send Pending)
	// 16:    Mask (0: Enabled, 1: Disabled)
	// 0x320
	tReg	LVTTimer;	// LVT Timer Register (RW)
	tReg	LVTThermalSensor;	// LVT Thermal Sensor Register (RW)
	tReg	LVTPerfMonCounters;	// LVT Performance Monitor Counters Register (RW)
	tReg	LVTLInt0;	// LVT Local Interrupt (LINT) #0 Register (RW);
	tReg	LVTLInt1;	// LVT Local Interrupt (LINT) #1 Register (RW);
	tReg	LVTError;	// LVT Error Register (RW);
	// 0x380
	tReg	InitialCount;	// Initial Count Register (Used for the timer) (RW)
	tReg	CurrentCount;	// Current Count Register (Used for the timer) (RW)
	tReg	_unused5[4];
	// 0x3E0
	tReg	DivideConifg;	// Divide Configuration Register (RW)
	tReg	_unused6[1];
} volatile	tAPIC;

#endif
