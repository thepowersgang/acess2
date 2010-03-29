/*
 * AcessOS Microkernel Version
 * mboot.h
 */
#ifndef _MBOOT_H
#define _MBOOT_H

#define MULTIBOOT_MAGIC	0x2BADB002

// === TYPES ===
typedef struct {
	Uint32	Flags;
	Uint32	LowMem;
	Uint32	HighMem;
	Uint32	BootDevice;
	Uint32	CommandLine;
	Uint32	ModuleCount;
	Uint32	Modules;
	Uint32	SymbolInfo[4];	// #32 UNUSED
	Uint32	MMapLength;
	Uint32	MMapAddr;		// #40
} tMBoot_Info;

typedef struct {
	Uint32	Start;
	Uint32	End;
	char	*String;
	Uint32	Resvd;
} tMBoot_Module;

typedef struct {
	Uint32	Size;	// (May be at offset -4)
	Uint64	Base;
	Uint64	Length;
	Uint32	Type;	//1:RAM,Else Reserved
} __attribute__ ((packed)) tMBoot_MMapEnt;

#endif
