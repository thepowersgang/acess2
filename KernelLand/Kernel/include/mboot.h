/*
 * AcessOS Microkernel Version
 * mboot.h
 */
#ifndef _MBOOT_H
#define _MBOOT_H
#include <acess.h>

#define MULTIBOOT_MAGIC	0x2BADB002

#include <pmemmap.h>
#include <bootmod.h>

// === TYPES ===
typedef struct {
	Uint32	Flags;
	// flags[0]
	Uint32	LowMem;
	Uint32	HighMem;
	// flags[1]
	Uint32	BootDevice;
	// flags[2]
	Uint32	CommandLine;
	// flags[3]
	Uint32	ModuleCount;
	Uint32	Modules;
	// flags[4] or flags[5]
	Uint32	SymbolInfo[4];	// #32 UNUSED
	// flags[6]
	Uint32	MMapLength;
	Uint32	MMapAddr;		// #40
	// flags[7]
	Uint32	drives_length;
	Uint32	drives_addr;
	// flags[8]
	Uint32	config_table;
	// flags[9]
	Uint32	boot_loader_name;
	// flags[10]
	Uint32	apm_table;
	// flags[11]
	Uint32	vbe_control_info;
	Uint32	vbe_mode_info;
	Uint32	vbe_mode;
	Uint32	vbe_interface_seg;
	Uint32	vbe_interface_off;
	Uint32	vbe_interface_len;
} tMBoot_Info;

typedef struct {
	Uint32	Start;
	Uint32	End;
	Uint32	String;
	Uint32	Resvd;
} tMBoot_Module;

typedef struct {
	Uint32	Size;	// (May be at offset -4)
	Uint64	Base;
	Uint64	Length;
	Uint32	Type;	//1:RAM,Else Reserved
} __attribute__ ((packed)) tMBoot_MMapEnt;

extern int	Multiboot_LoadMemoryMap(tMBoot_Info *MBInfo, tVAddr MapOffset, tPMemMapEnt *Map, const int MapSize, tPAddr KStart, tPAddr KEnd);
extern tBootModule	*Multiboot_LoadModules(tMBoot_Info *MBInfo, tVAddr MapOffset, int *ModuleCount);
extern void	Multiboot_FreeModules(const int ModuleCount, tBootModule *Modules);

#endif
