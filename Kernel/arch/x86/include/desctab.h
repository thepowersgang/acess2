/**
 */
#ifndef _DESCTAB_H_
#define _DESCTAB_H_

typedef struct {
	Uint16	LimitLow;
	Uint16	BaseLow;
	Uint8	BaseMid;
	Uint8	Access;
	struct {
		unsigned LimitHi:	4;
		unsigned Flags:		4;
	} __attribute__ ((packed));
	Uint8	BaseHi;
} __attribute__ ((packed)) tGDT;

typedef struct {
	Uint16	OffsetLo;
	Uint16	CS;
	Uint16	Flags;
	Uint16	OffsetHi;
} __attribute__ ((packed)) tIDT;

#endif
