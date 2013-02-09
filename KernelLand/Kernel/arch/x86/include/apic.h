/*
 * Acess2 Kernel (x86 Port)
 * - By John Hodge (thePowersGang)
 *
 * include/apic.h
 * - Definitions for APIC/IOAPIC
 */
#ifndef _APIC_H_
#define _APIC_H_

typedef volatile struct sIOAPIC	tIOAPIC;
typedef volatile struct sAPICReg tAPICReg;
typedef struct sAPIC	tAPIC;

extern tAPIC	*gpMP_LocalAPIC;

struct sIOAPIC
{
	Uint32	Addr;
	Uint32	Resvd1[3];
	union {
		Uint8	Byte;
		Uint16	Word;
		Uint32	DWord;
	}	Value;
	Uint32	Resvd2[3];
};

struct sAPICReg
{
	Uint32	Val;
	Uint32	Padding[3];
};

struct sAPIC
{
	tAPICReg	_unused1[2];
	tAPICReg	ID;
	tAPICReg	Version;
	tAPICReg	_unused2[4];
	tAPICReg	TPR;	// Task Priority Register
	tAPICReg	APR;	// Arbitration Priority Register (RO)
	tAPICReg	PPR;	// Processor Priority Register (RO)
	tAPICReg	EOI;	// EOI Register (Write Only)
	tAPICReg	_unused3[1];
	tAPICReg	LogDest;	// Logical Destination Register
	tAPICReg	DestFmt;	// Destination Format Register (0-27: RO, 28-31: RW)
	tAPICReg	SIV;	// Spurious Interrupt Vector Register (0-8: RW, 9-31: RO)
	tAPICReg	ISR[8];	// In-Service Register - Total 256 Bits (RO)
	tAPICReg	TMR[8];	// Trigger Mode Register - Total 256 Bits (RO)
	tAPICReg	IRR[8];	// Interrupt Request Register - Total 256 Bits (RO)
	tAPICReg	ErrorStatus;	// Error Status Register (RO)
	tAPICReg	_unused4[6];
	tAPICReg	LVTCMI;	// LVT CMI Registers
	// 0x300
	tAPICReg	ICR[2];	// Interrupt Command Register (RW)
	// LVT Registers (Controls Local Vector Table)
	// Structure:
	// 0-7:   Vector - IDT Vector for the interrupt
	// 12:    Delivery Status (0: Idle, 1: Send Pending)
	// 16:    Mask (0: Enabled, 1: Disabled)
	// 0x320
	tAPICReg	LVTTimer;	// LVT Timer Register (RW)
	tAPICReg	LVTThermalSensor;	// LVT Thermal Sensor Register (RW)
	tAPICReg	LVTPerfMonCounters;	// LVT Performance Monitor Counters Register (RW)
	tAPICReg	LVTLInt0;	// LVT Local Interrupt (LINT) #0 Register (RW);
	tAPICReg	LVTLInt1;	// LVT Local Interrupt (LINT) #1 Register (RW);
	tAPICReg	LVTError;	// LVT Error Register (RW);
	// 0x380
	tAPICReg	InitialCount;	// Initial Count Register (Used for the timer) (RW)
	tAPICReg	CurrentCount;	// Current Count Register (Used for the timer) (RW)
	tAPICReg	_unused5[4];
	// 0x3E0
	tAPICReg	DivideConifg;	// Divide Configuration Register (RW)
	tAPICReg	_unused6[1];
};

#endif
