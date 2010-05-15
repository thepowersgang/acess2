/**
 */
#ifndef _DESCTAB_H_
#define _DESCTAB_H_

typedef struct {
	union
	{
		struct
		{
			Uint16	LimitLow;
			Uint16	BaseLow;
			Uint8	BaseMid;
			Uint8	Access;
			struct {
				unsigned LimitHi:	4;
				unsigned Flags:		4;
			} __attribute__ ((packed));
			Uint8	BaseHi;
		};
		Uint32	DWord[2];
	};
} __attribute__ ((packed)) tGDT;

typedef struct {
	Uint16	OffsetLo;
	Uint16	CS;
	Uint16	Flags;	// 0-2: IST, 3-7: 0, 8-11: Type, 12: 0, 13-14: DPL, 15: Present
	Uint16	OffsetMid;
	Uint32	OffsetHi;
	Uint32	Reserved;
} __attribute__ ((packed)) tIDT;

typedef struct {
	Uint32	Rsvd1;
	
	Uint64	RSP0;
	Uint64	RSP1;
	Uint64	RSP2;
	
	Uint32	Rsvd2[2];
	
	// Interrupt Stack Table Pointers
	Uint64	IST1;
	Uint64	IST2;
	Uint64	IST3;
	Uint64	IST4;
	Uint64	IST5;
	Uint64	IST6;
	Uint64	IST7;
	
	Uint32	Rsvd3[2];
	Uint16	Rsvd4;
	Uint16	IOMapBase;
	
	// Acess Specific Fields
	Uint8	CPUNumber;	// CPU Number
} __attribute__ ((packed)) tTSS;

#endif
