/*
 * Acess2 USB Stack Mass Storage Driver
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Driver Core
 */
#ifndef _MSC__MSC_PROTO_H_
#define _MSC__MSC_PROTO_H_

typedef struct sMSC_CBW	tMSC_CBW;
typedef struct sMSC_CSW	tMSC_CSW;

struct sMSC_CBW
{
	Uint32	dCBWSignature;	// = 0x43425355
	Uint32	dCBWTag;
	Uint32	dCBWDataTransferLength;
	Uint8	bmCBWFlags;
	Uint8	bCBWLUN;
	Uint8	bCBWLength;
	Uint8	CBWCB[16];
};

struct sMSC_CSW
{
	Uint32	dCSWSignature;	// = 0x53425355
	Uint32	dCSWTag;
	Uint32	dCSWDataResidue;
	Uint8	dCSWStatus;
};

#endif
