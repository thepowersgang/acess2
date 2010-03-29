/*
 * Acess 2
 * By John Hodge (thePowersGang)
 *
 * multiboot2.h
 * - Multiboot 2 Header
 */
#ifndef _MULTIBOOT2_H_
#define _MULTIBOOT2_H_

#define MULTIBOOT2_MAGIC	0x36D76289

typedef struct sMultiboot2Info
{
	Uint32	TotalSize;
	Uint32	Reserved;	// SBZ
}	tMultiboot2Info;

#endif
