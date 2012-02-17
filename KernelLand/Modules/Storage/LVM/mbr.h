/*
 * Acess2 Logical Volume Manager
 * - By John Hodge (thePowersGang)
 * 
 * mbr.h
 * - MBR Definitions
 */
#ifndef _LVM_MBR_H_
#define _LVM_MBR_H_

typedef struct
{
	Uint8	BootCode[0x1BE];
	struct {
		Uint8	Boot;
		Uint8	Unused1;	// Also CHS Start
		Uint16	StartHi;	// Also CHS Start
		Uint8	SystemID;
		Uint8	Unused2;	// Also CHS Length
		Uint16	LengthHi;	// Also CHS Length
		Uint32	LBAStart;
		Uint32	LBALength;
	} __attribute__ ((packed)) Parts[4];
	Uint16	BootFlag;	// = 0xAA 55
} __attribute__ ((packed))	tMBR;

#endif
