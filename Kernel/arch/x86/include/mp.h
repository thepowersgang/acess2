/*
 */
#ifndef _MP_H
#define _MP_H

#define MPTABLE_IDENT	('_'|('M'<<8)|('P'<<16)|('_'<<24))

typedef struct sMPInfo {
	Uint	Sig;	// '_MP_'
	Uint	MPConfig;
	Uint8	Length;
	Uint8	Version;
	Uint8	Checksum;
	Uint8	Features[5];	// 2-4 are unused
} tMPInfo;

#endif
