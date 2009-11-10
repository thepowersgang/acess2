/*
 * AcessOS Microkernel Version
 * common.h
 */
#ifndef _COMMON_H
#define _COMMON_H

#define NULL	((void*)0)

#include <arch.h>
#include <stdarg.h>

// --- Helper Macros ---
/**
 * \name Helper Macros
 * \{
 */
#define	CONCAT(x,y) x ## y
#define EXPAND_CONCAT(x,y) CONCAT(x,y)
#define STR(x) #x
#define EXPAND_STR(x) STR(x)
/**
 * \}
 */

/**
 * \name Per-Process Configuration Settings
 * \{
 */
enum eConfigTypes {
	CFGT_NULL,
	CFGT_INT,
	CFGT_HEAPSTR,
	CFGT_PTR
};
enum eConfigs {
	CFG_VFS_CWD,
	CFG_VFS_MAXFILES,
	NUM_CFG_ENTRIES
};
#define CFGINT(id)	(*Threads_GetCfgPtr(id))
#define CFGPTR(id)	(*(void**)Threads_GetCfgPtr(id))
/**
 * \}
 */

// === CONSTANTS ===
// --- Memory Flags --
/**
 * \name Memory Flags
 * \{
 * \todo Move to mm_virt.h
 */
#define	MM_PFLAG_RO		0x01	// Writes disallowed
#define	MM_PFLAG_EXEC	0x02	// Allow execution
#define	MM_PFLAG_NOPAGE	0x04	// Prevent from being paged out
#define	MM_PFLAG_COW	0x08	// Copy-On-Write
#define	MM_PFLAG_KERNEL	0x10	// Kernel-Only (Ring0)
/**
 * \}
 */

// === Kernel Export Macros ===
/**
 * \name Kernel Function 
 * \{
 */
typedef struct sKernelSymbol {
	char	*Name;
	unsigned int	Value;
} tKernelSymbol;
#define	EXPORT(_name)	tKernelSymbol _kexp_##_name __attribute__((section ("KEXPORT"),unused))={#_name, (Uint)_name}
#define	EXPORTV(_name)	tKernelSymbol _kexp_##_name __attribute__((section ("KEXPORT"),unused))={#_name, (Uint)&_name}
#define	EXPORTAS(_sym,_name)	tKernelSymbol _kexp_##_name __attribute__((section ("KEXPORT"),unused))={#_name, (Uint)_sym}
/**
 * \}
 */

// === FUNCTIONS ===
// --- Core ---
extern void	System_Init(char *ArgString);

// --- IRQs ---
extern int	IRQ_AddHandler(int Num, void (*Callback)(int));

// --- Debug ---
/**
 * \name Debugging and Errors
 * \{
 */
extern void	Panic(char *Msg, ...);
extern void	Warning(char *Msg, ...);
extern void	Log(char *Fmt, ...);
extern void	LogV(char *Fmt, va_list Args);
extern void	LogF(char *Fmt, ...);
extern void	Debug_Enter(char *FuncName, char *ArgTypes, ...);
extern void	Debug_Log(char *FuncName, char *Fmt, ...);
extern void	Debug_Leave(char *FuncName, char RetType, ...);
extern void	Debug_HexDump(char *Header, void *Data, Uint Length);
#if DEBUG
# define ENTER(_types...)	Debug_Enter((char*)__func__, _types)
# define LOG(_fmt...)	Debug_Log((char*)__func__, _fmt)
# define LEAVE(_t...)	Debug_Leave((char*)__func__, _t)
#else
# define ENTER(...)
# define LOG(...)
# define LEAVE(...)
#endif
/**
 * \}
 */

// --- IO ---
/**
 * \name I/O Memory Access
 * \{
 */
extern void	outb(Uint16 Port, Uint8 Data);
extern void	outw(Uint16 Port, Uint16 Data);
extern void	outd(Uint16 Port, Uint32 Data);
extern void	outq(Uint16 Port, Uint64 Data);
extern Uint8	inb(Uint16 Port);
extern Uint16	inw(Uint16 Port);
extern Uint32	ind(Uint16 Port);
extern Uint64	inq(Uint16 Port);
/**
 * \}
 */

// --- Memory Management ---
/**
 * \name Memory Management
 * \{
 * \todo Move to mm_virt.h
 */
/**
 * \brief Allocate a physical page at \a VAddr
 * \param VAddr	Virtual Address to allocate at
 * \return Physical address allocated
 */
extern tPAddr	MM_Allocate(tVAddr VAddr);
/**
 * \brief Deallocate a page
 * \param VAddr	Virtual address to unmap
 */
extern void	MM_Deallocate(tVAddr VAddr);
/**
 * \brief Map a physical page at \a PAddr to \a VAddr
 * \param VAddr	Target virtual address
 * \param PAddr	Physical address to map
 * \return Boolean Success
 */
extern int	MM_Map(tVAddr VAddr, tPAddr PAddr);
/**
 * \brief Get the physical address of \a VAddr
 * \param VAddr	Address of the page to get the physical address of
 * \return Physical page mapped at \A VAddr
 */
extern tPAddr	MM_GetPhysAddr(tVAddr VAddr);
/**
 * \brief Checks is a memory range is user accessable
 * \param VAddr	Base address to check
 * \param Length	Number of bytes to check
 * \return 1 if the memory is all user-accessable, 0 otherwise
 */
extern int	MM_IsUser(tVAddr VAddr, int Length);
/**
 * \brief Set the access flags on a page
 * \param VAddr	Virtual address of the page
 * \param Flags New flags value
 * \param Mask	Flags to set
 */
extern void	MM_SetFlags(tVAddr VAddr, Uint Flags, Uint Mask);
/**
 * \brief Temporarily map a page into the address space
 * \param PAddr	Physical addres to map
 * \return Virtual address of page in memory
 * \note There is only a limited ammount of slots avaliable
 */
extern tVAddr	MM_MapTemp(tPAddr PAddr);
/**
 * \brief Free a temporarily mapped page
 * \param VAddr	Allocate virtual addres of page
 */
extern void	MM_FreeTemp(tVAddr VAddr);
/**
 * \brief Map a physcal address range into the virtual address space
 * \param PAddr	Physical address to map in
 * \param Number	Number of pages to map
 */
extern tVAddr	MM_MapHWPage(tPAddr PAddr, Uint Number);
/**
 * \brief Allocates DMA physical memory
 * \param Pages	Number of pages required
 * \param MaxBits	Maximum number of bits the physical address can have
 * \param PhysAddr	Pointer to the location to place the physical address allocated
 * \return Virtual address allocate
 */
extern tVAddr	MM_AllocDMA(int Pages, int MaxBits, tPAddr *PhysAddr);
/**
 * \brief Unmaps an allocated hardware range
 * \param VAddr	Virtual address allocate by ::MM_MapHWPage or ::MM_AllocDMA
 * \param Number	Number of pages to free
 */
extern void	MM_UnmapHWPage(tVAddr VAddr, Uint Number);
/**
 * \brief Allocate a single physical page
 * \return Physical address allocated
 */
extern tPAddr	MM_AllocPhys();
/**
 * \brief Allocate a contiguous range of physical pages
 * \param Pages	Number of pages to allocate
 * \return First physical address allocated
 */
extern tPAddr	MM_AllocPhysRange(int Pages);
/**
 * \brief Reference a physical page
 * \param PAddr	Page to mark as referenced
 */
extern void	MM_RefPhys(tPAddr PAddr);
/**
 * \brief Dereference a physical page
 * \param PAddr	Page to dereference
 */
extern void	MM_DerefPhys(tPAddr PAddr);
/**
 * \}
 */

// --- Memory Manipulation ---
/**
 * \name Memory Manipulation
 * \{
 */
extern int	memcmp(const void *m1, const void *m2, Uint count);
extern void *memcpy(void *dest, const void *src, Uint count);
extern void *memcpyd(void *dest, const void *src, Uint count);
extern void *memset(void *dest, int val, Uint count);
extern void *memsetd(void *dest, Uint val, Uint count);
/**
 * \}
 */

// --- Endianness ---
/**
 * \name Endianness Swapping
 * \{
 */
extern Uint16	LittleEndian16(Uint16 Val);
extern Uint16	BigEndian16(Uint16 Val);
extern Uint32	LittleEndian32(Uint32 Val);
extern Uint32	BigEndian32(Uint32 Val);
/**
 * \}
 */

// --- Strings ---
/**
 * \name Strings
 * \{
 */
extern Uint	strlen(const char *Str);
extern char	*strcpy(char *__dest, const char *__src);
extern int	strcmp(const char *__str1, const char *__str2);
extern int	strncmp(const char *Str1, const char *Str2, size_t num);
extern int	strucmp(const char *Str1, const char *Str2);
extern char	*strdup(const char *Str);
extern int	strpos(const char *Str, char Ch);
extern int	strpos8(const char *str, Uint32 search);
extern void	itoa(char *buf, Uint num, int base, int minLength, char pad);
extern int	ReadUTF8(Uint8 *str, Uint32 *Val);
extern int	WriteUTF8(Uint8 *str, Uint32 Val);
/**
 * \}
 */

extern Uint	rand();

// --- Heap ---
/**
 * \name Heap
 * \{
 */
extern void *malloc(size_t size);
extern void *calloc(size_t num, size_t size);
extern void	*realloc(void *ptr, size_t size);
extern void free(void *Ptr);
extern int	IsHeap(void *Ptr);
/**
 * \}
 */

// --- Modules ---
/**
 * \name Modules
 * \{
 */
extern int	Module_LoadMem(void *Buffer, Uint Length, char *ArgStr);
extern int	Module_LoadFile(char *Path, char *ArgStr);
/**
 * \}
 */

// --- Timing ---
/**
 * \name Time and Timing
 * \{
 */
extern Sint64	timestamp(int sec, int mins, int hrs, int day, int month, int year);
extern Sint64	now();
extern int	Time_CreateTimer(int Delta, void *Callback, void *Argument);
extern void	Time_RemoveTimer(int ID);
extern void	Time_Delay(int Delay);
/**
 * \}
 */

// --- Threads ---
/**
 * \name Threads and Processes
 * \{
 */
extern  int	Proc_SpawnWorker();
extern  int	Proc_Spawn(char *Path);
extern void	Threads_Exit();
extern void	Threads_Yield();
extern void	Threads_Sleep();
extern int	Threads_GetUID();
extern int	Threads_GetGID();
extern int	SpawnTask(tThreadFunction Function, void *Arg);
extern Uint	*Threads_GetCfgPtr(int Id);
/**
 * \}
 */

// --- Simple Math ---
extern int	DivUp(int num, int dem);

#include <binary_ext.h>
#include <vfs_ext.h>

#endif
