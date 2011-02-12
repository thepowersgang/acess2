/*
 * AcessOS Microkernel Version
 * acess.h
 */
#ifndef _ACESS_H
#define _ACESS_H

#define NULL	((void*)0)
#define PACKED	__attribute__((packed))
#define UNUSED(x)	UNUSED_##x __attribute__((unused))
#define offsetof(st, m) ((Uint)((char *)&((st *)(0))->m - (char *)0 ))

#include <arch.h>
#include <stdarg.h>
#include "errno.h"

// --- Types ---
typedef  int	tPID;
typedef  int	tTID;
typedef Uint	tUID;
typedef Uint	tGID;
typedef Sint64	tTimestamp;
typedef struct sShortSpinlock	tShortSpinlock;
typedef struct sMutex	tMutex;
typedef struct sSemaphore	tSemaphore;

struct sMutex {
	tShortSpinlock	Protector;	//!< Protector for the lock strucure
	const char	*Name;	//!< Human-readable name
	struct sThread	*volatile Owner;	//!< Owner of the lock (set upon getting the lock)
	struct sThread	*Waiting;	//!< Waiting threads
	struct sThread	*LastWaiting;	//!< Waiting threads
};

struct sSemaphore {
	tShortSpinlock	Protector;	//!< Protector for the lock strucure
	const char	*Name;	//!< Human-readable name
	volatile int	Value;	//!< Current mutex value
	struct sThread	*Waiting;	//!< Waiting threads
	struct sThread	*LastWaiting;	//!< Waiting threads
};

// --- Helper Macros ---
/**
 * \name Helper Macros
 * \{
 */
#define	CONCAT(x,y) x ## y
#define EXPAND_CONCAT(x,y) CONCAT(x,y)
#define STR(x) #x
#define EXPAND_STR(x) STR(x)

#define VER2(major,minor)	((((major)&0xFF)<<8)|((minor)&0xFF))
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
	CFG_VFS_CHROOT,
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
// --- Interface Flags & Macros
#define CLONE_VM	0x10

// === Types ===
typedef void (*tThreadFunction)(void*);

// === Kernel Export Macros ===
/**
 * \name Kernel Function 
 * \{
 */
typedef struct sKernelSymbol {
	char	*Name;
	tVAddr	Value;
} tKernelSymbol;
#define	EXPORT(_name)	tKernelSymbol _kexp_##_name __attribute__((section ("KEXPORT"),unused))={#_name, (tVAddr)_name}
#define	EXPORTV(_name)	tKernelSymbol _kexp_##_name __attribute__((section ("KEXPORT"),unused))={#_name, (tVAddr)&_name}
#define	EXPORTAS(_sym,_name)	tKernelSymbol _kexp_##_name __attribute__((section ("KEXPORT"),unused))={#_name, (tVAddr)_sym}
/**
 * \}
 */

// === FUNCTIONS ===
// --- IRQs ---
extern int	IRQ_AddHandler(int Num, void (*Callback)(int));

// --- Logging ---
extern void	Log_KernelPanic(char *Ident, char *Message, ...);
extern void	Log_Panic(char *Ident, char *Message, ...);
extern void	Log_Error(char *Ident, char *Message, ...);
extern void	Log_Warning(char *Ident, char *Message, ...);
extern void	Log_Notice(char *Ident, char *Message, ...);
extern void	Log_Log(char *Ident, char *Message, ...);
extern void	Log_Debug(char *Ident, char *Message, ...);

// --- Debug ---
/**
 * \name Debugging and Errors
 * \{
 */
extern void	Debug_KernelPanic(void);	//!< Initiate a kernel panic
extern void	Panic(const char *Msg, ...);	//!< Print a panic message (initiates a kernel panic)
extern void	Warning(const char *Msg, ...);	//!< Print a warning message
extern void	LogF(const char *Fmt, ...);	//!< Print a log message without a trailing newline
extern void	Log(const char *Fmt, ...);	//!< Print a log message
extern void	Debug(const char *Fmt, ...);	//!< Print a debug message (doesn't go to KTerm)
extern void	LogV(const char *Fmt, va_list Args);	//!< va_list Log message
extern void	Debug_Enter(const char *FuncName, const char *ArgTypes, ...);
extern void	Debug_Log(const char *FuncName, const char *Fmt, ...);
extern void	Debug_Leave(const char *FuncName, char RetType, ...);
extern void	Debug_HexDump(const char *Header, const void *Data, Uint Length);
#define UNIMPLEMENTED()	Warning("'%s' unimplemented", __func__)
#if DEBUG
# define ENTER(_types...)	Debug_Enter((char*)__func__, _types)
# define LOG(_fmt...)	Debug_Log((char*)__func__, _fmt)
# define LEAVE(_t...)	Debug_Leave((char*)__func__, _t)
# define LEAVE_RET(_t,_v...)	do{LEAVE(_t,_v);return _v;}while(0)
# define LEAVE_RET0()	do{LEAVE('-');return;}while(0)
#else
# define ENTER(...)
# define LOG(...)
# define LEAVE(...)
# define LEAVE_RET(_t,_v...)	return (_v)
# define LEAVE_RET0()	return
#endif
#if SANITY
# define ASSERT(expr) do{if(!(expr))Panic("%s: Assertion '"#expr"' failed",(char*)__func__);}while(0)
#else
# define ASSERT(expr)
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
 * \brief Get the physical address of \a Addr
 * \param Addr	Address of the page to get the physical address of
 * \return Physical page mapped at \a Addr
 */
extern tPAddr	MM_GetPhysAddr(tVAddr Addr);
/**
 * \brief Set the access flags on a page
 * \param VAddr	Virtual address of the page
 * \param Flags New flags value
 * \param Mask	Flags to set
 */
extern void	MM_SetFlags(tVAddr VAddr, Uint Flags, Uint Mask);
/**
 * \brief Get the flags on a flag
 * \param VAddr	Virtual address of page
 * \return Flags value of the page
 */
extern Uint	MM_GetFlags(tVAddr VAddr);
/**
 * \brief Checks is a memory range is user accessable
 * \param VAddr	Base address to check
 * \return 1 if the memory is all user-accessable, 0 otherwise
 */
#define MM_IsUser(VAddr)	(!(MM_GetFlags((VAddr))&MM_PFLAG_KERNEL))
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
extern tVAddr	MM_MapHWPages(tPAddr PAddr, Uint Number);
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
extern void	MM_UnmapHWPages(tVAddr VAddr, Uint Number);
/**
 * \brief Allocate a single physical page
 * \return Physical address allocated
 */
extern tPAddr	MM_AllocPhys(void);
/**
 * \brief Allocate a contiguous range of physical pages
 * \param Pages	Number of pages to allocate
 * \param MaxBits	Maximum number of address bits allowed
 * \return First physical address allocated
 */
extern tPAddr	MM_AllocPhysRange(int Pages, int MaxBits);
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
extern int	memcmp(const void *m1, const void *m2, size_t count);
extern void *memcpy(void *dest, const void *src, size_t count);
extern void *memcpyd(void *dest, const void *src, size_t count);
extern void *memset(void *dest, int val, size_t count);
extern void *memsetd(void *dest, Uint32 val, size_t count);
/**
 * \}
 */
/**
 * \name Memory Validation
 * \{
 */
extern int	CheckString(char *String);
extern int	CheckMem(void *Mem, int Num);
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
extern int	vsnprintf(char *__s, size_t __maxlen, const char *__format, va_list args);
extern int	sprintf(char *__s, const char *__format, ...);
extern size_t	strlen(const char *Str);
extern char	*strcpy(char *__dest, const char *__src);
extern char	*strncpy(char *__dest, const char *__src, size_t max);
extern int	strcmp(const char *__str1, const char *__str2);
extern int	strncmp(const char *Str1, const char *Str2, size_t num);
extern int	strucmp(const char *Str1, const char *Str2);
//extern char	*strdup(const char *Str);
#define strdup(Str)	_strdup(_MODULE_NAME_"/"__FILE__, __LINE__, (Str))
extern char	*_strdup(const char *File, int Line, const char *Str);
extern char	**str_split(const char *__str, char __ch);
extern char	*strchr(const char *__s, int __c);
extern int	strpos(const char *Str, char Ch);
extern int	strpos8(const char *str, Uint32 search);
extern void	itoa(char *buf, Uint64 num, int base, int minLength, char pad);
extern int	atoi(const char *string);
extern int	ReadUTF8(Uint8 *str, Uint32 *Val);
extern int	WriteUTF8(Uint8 *str, Uint32 Val);
extern int	ModUtil_SetIdent(char *Dest, char *Value);
extern int	ModUtil_LookupString(char **Array, char *Needle);

extern Uint8	ByteSum(void *Ptr, int Size);
extern int	UnHex(Uint8 *Dest, size_t DestSize, const char *SourceString);
/**
 * \}
 */

extern int	rand(void);
extern int	CallWithArgArray(void *Function, int NArgs, Uint *Args);

// --- Heap ---
#include <heap.h>

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
/**
 * \brief Create a timestamp from a time
 */
extern Sint64	timestamp(int sec, int mins, int hrs, int day, int month, int year);
/**
 * \brief Gets the current timestamp (miliseconds since Midnight 1st January 1970)
 */
extern Sint64	now(void);
/**
 * \brief Timer callback function
 */
typedef void (tTimerCallback)(void *);
/**
 * \brief Creates a one-shot timer
 * \param Delta	Period of the timer
 * \param Callback	Function to call each time
 * \param Argument	Argument to pass to the callback
 */
extern int	Time_CreateTimer(int Delta, tTimerCallback *Callback, void *Argument);
/**
 * \brief Removed an active timer
 */
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
extern int	Proc_SpawnWorker(void);
extern int	Proc_Spawn(char *Path);
extern void	Threads_Exit(int TID, int Status);
extern void	Threads_Yield(void);
extern void	Threads_Sleep(void);
extern int	Threads_WakeTID(tTID Thread);
extern tPID	Threads_GetPID(void);
extern tTID	Threads_GetTID(void);
extern tUID	Threads_GetUID(void);
extern tGID	Threads_GetGID(void);
extern int	SpawnTask(tThreadFunction Function, void *Arg);
extern Uint	*Threads_GetCfgPtr(int Id);
extern int	Threads_SetName(const char *NewName);
extern void	Mutex_Acquire(tMutex *Mutex);
extern void	Mutex_Release(tMutex *Mutex);
extern int	Mutex_IsLocked(tMutex *Mutex);
/**
 * \}
 */

// --- Simple Math ---
extern int	DivUp(int num, int dem);

#include <binary_ext.h>
#include <vfs_ext.h>
#include <adt.h>

#endif
