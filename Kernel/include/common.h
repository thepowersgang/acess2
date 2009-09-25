/*
 * AcessOS Microkernel Version
 * common.h
 */
#ifndef _COMMON_H
#define _COMMON_H

#define NULL	((void*)0)

#include <arch.h>
#include <stdarg.h>

enum eConfigs {
	CFG_VFS_CWD,
	CFG_VFS_MAXFILES,
	NUM_CFG_ENTRIES
};
#define CFGINT(_idx)	(*(Uint*)(MM_PPD_CFG+(_idx)*sizeof(void*)))
#define CFGPTR(_idx)	(*(void**)(MM_PPD_CFG+(_idx)*sizeof(void*)))

// === CONSTANTS ===
// --- Memory Flags --
#define	MM_PFLAG_RO		0x01	// Writes disallowed
#define	MM_PFLAG_EXEC	0x02	// Allow execution
#define	MM_PFLAG_NOPAGE	0x04	// Prevent from being paged out
#define	MM_PFLAG_COW	0x08	// Copy-On-Write
#define	MM_PFLAG_KERNEL	0x10	// Kernel-Only (Ring0)

// === Kernel Export Macros ===
typedef struct sKernelSymbol {
	char	*Name;
	unsigned int	Value;
} tKernelSymbol;
#define	EXPORT(_name)	tKernelSymbol _kexp_##_name __attribute__((section ("KEXPORT"),unused))={#_name, (Uint)_name}
#define	EXPORTAS(_sym,_name)	tKernelSymbol _kexp_##_name __attribute__((section ("KEXPORT"),unused))={#_name, (Uint)_sym}

// === FUNCTIONS ===
// --- Core ---
extern void	System_Init(char *ArgString);
extern int	IRQ_AddHandler(int Num, void (*Callback)(void));
// --- Debug ---
extern void	Panic(char *Msg, ...);
extern void	Warning(char *Msg, ...);
extern void	Log(char *Fmt, ...);
extern void	LogV(char *Fmt, va_list Args);
extern void	Debug_Enter(char *FuncName, char *ArgTypes, ...);
extern void	Debug_Log(char *FuncName, char *Fmt, ...);
extern void	Debug_Leave(char *FuncName, char RetType, ...);
extern void	Debug_HexDump(char *Header, void *Data, Uint Length);
#define ENTER(_types...)	Debug_Enter((char*)__func__, _types)
#define LOG(_fmt...)	Debug_Log((char*)__func__, _fmt)
#define LEAVE(_t...)	Debug_Leave((char*)__func__, _t)
// --- IO ---
extern void	outb(Uint16 Port, Uint8 Data);
extern void	outw(Uint16 Port, Uint16 Data);
extern void	outd(Uint16 Port, Uint32 Data);
extern void	outq(Uint16 Port, Uint64 Data);
extern Uint8	inb(Uint16 Port);
extern Uint16	inw(Uint16 Port);
extern Uint32	ind(Uint16 Port);
extern Uint64	inq(Uint16 Port);
// --- Memory ---
extern tPAddr	MM_Allocate(Uint VAddr);
extern void	MM_Deallocate(Uint VAddr);
extern int	MM_Map(Uint VAddr, tPAddr PAddr);
extern tPAddr	MM_GetPhysAddr(Uint VAddr);
extern void	MM_SetFlags(Uint VAddr, Uint Flags, Uint Mask);
extern Uint	MM_MapTemp(tPAddr PAddr);
extern void	MM_FreeTemp(Uint PAddr);
extern Uint	MM_MapHWPage(tPAddr PAddr, Uint Number);
extern void	MM_UnmapHWPage(Uint VAddr, Uint Number);
extern tPAddr	MM_AllocPhys();
extern void	MM_RefPhys(tPAddr Addr);
extern void	MM_DerefPhys(tPAddr Addr);
extern void *memcpy(void *dest, void *src, Uint count);
extern void *memcpyd(void *dest, void *src, Uint count);
extern void *memset(void *dest, int val, Uint count);
extern void *memsetd(void *dest, Uint val, Uint count);
// --- Strings ---
extern Uint	strlen(char *Str);
extern char	*strcpy(char *__dest, char *__src);
extern int	strcmp(char *__dest, char *__src);
extern int	strncmp(char *Str1, char *Str2, size_t num);
extern int	strucmp(char *Str1, char *Str2);
extern int	strpos(char *Str, char Ch);
extern int	strpos8(char *str, Uint32 search);
extern void	itoa(char *buf, Uint num, int base, int minLength, char pad);
extern int	ReadUTF8(Uint8 *str, Uint32 *Val);
extern int	WriteUTF8(Uint8 *str, Uint32 Val);
extern Uint	rand();
// --- Heap ---
extern void *malloc(size_t size);
extern void	*realloc(void *ptr, size_t size);
extern void free(void *Ptr);
extern int	IsHeap(void *Ptr);
// --- Modules ---
extern int	Module_LoadMem(void *Buffer, Uint Length, char *ArgStr);
extern int	Module_LoadFile(char *Path, char *ArgStr);
// --- Timing ---
extern Sint64	timestamp(int sec, int mins, int hrs, int day, int month, int year);
extern Sint64	now();
// --- Threads ---
extern  int	Proc_Spawn(char *Path);
extern void	Threads_Exit();
extern void	Threads_Yield();
extern void	Threads_Sleep();
extern int	Threads_GetCfg(int Index);
extern int	Threads_GetUID();
extern int	Threads_GetGID();
extern int	SpawnTask(tThreadFunction Function, void *Arg);

#include <binary_ext.h>
#include <vfs_ext.h>

#endif
