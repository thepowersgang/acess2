/*
 * AcessOS Microkernel Version
 * mboot.h
 */
#ifndef _MBOOT_H
#define _MBOOT_H

// === TYPES ===
typedef struct {
	Uint32	Flags;
	Uint32	LowMem;
	Uint32	HighMem;
	Uint32	BootDevice;
	Uint32	CommandLine;
	Uint32	ModuleCount;
	Uint32	Modules;
} tMBoot_Info;

typedef struct {
	Uint32	Start;
	Uint32	End;
	char	*String;
	Uint32	Resvd;
} tMBoot_Module;

#endif
