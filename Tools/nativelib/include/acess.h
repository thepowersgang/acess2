/*
 * Acess2 DiskTool utility
 * - By John Hodge (thePowersGang)
 *
 * include/acess.h
 * - Mock kernel core header
 */
#ifndef _DISKTOOL__ACESS_H_
#define _DISKTOOL__ACESS_H_

#define	CONCAT(x,y) x ## y
#define EXPAND_CONCAT(x,y) CONCAT(x,y)
#define STR(x) #x
#define EXPAND_STR(x) STR(x)

extern char	__buildnum[];
#define BUILD_NUM	((int)(Uint)&__buildnum)
extern const char gsGitHash[];
extern const char gsBuildInfo[];
#define VER2(major,minor)	((((major)&0xFF)<<8)|((minor)&0xFF))


#define BITS	32
#define NULL	((void*)0)
#include <stdint.h>
#include <stdbool.h>

typedef uintptr_t	Uint;
//typedef unsigned int	size_t;
#include <stddef.h>
typedef uint64_t	off_t;
typedef char	BOOL;


typedef uint8_t 	Uint8;
typedef uint16_t	Uint16;
typedef uint32_t	Uint32;
typedef uint64_t	Uint64;

typedef int8_t	Sint8;
typedef int16_t	Sint16;
typedef int32_t	Sint32;
typedef int64_t	Sint64;

typedef uintptr_t	tVAddr;
typedef uint32_t	tPAddr;

typedef uint32_t	tUID;
typedef uint32_t	tGID;
typedef uint32_t	tTID;
typedef uint32_t	tPGID;

typedef int64_t	tTime;
extern tTime	now(void);
extern int64_t	timestamp(int sec, int min, int hr, int day, int month, int year);
extern void	format_date(tTime TS, int *year, int *month, int *day, int *hrs, int *mins, int *sec, int *ms);

#define PACKED	__attribute__((packed))
#define NORETURN	__attribute__((noreturn))
#define DEPRECATED
#define EXPORT(s)
#define EXPORTV(s)

#include <vfs_ext.h>

// These are actually library functions, but they can't be included, so they're defined manually
extern void	*malloc(size_t bytes);
extern void	*calloc(size_t nmemb, size_t size);
extern void	*realloc(void *oldptr, size_t bytes);
extern void	free(void *buffer);
extern char	*strdup(const char *str);

#include <errno.h>
//#include <acess_logging.h>
#include <logdebug.h>

extern void	Debug_TraceEnter(const char *Function, const char *Format, ...);
extern void	Debug_TraceLog(const char *Function, const char *Format, ...);
extern void	Debug_TraceLeave(const char *Function, char Type, ...);
#undef ENTER
#undef LOG
#undef LEAVE
#undef LEAVE_RET
#if DEBUG
# define ENTER(str, v...)	Debug_TraceEnter(__func__, str, ##v)
# define LOG(fmt, v...) 	Debug_TraceLog(__func__, fmt, ##v)
# define LEAVE(t, v...)     	Debug_TraceLeave(__func__, t, ##v)
# define LEAVE_RET(t,v)	do{LEAVE('-');return v;}while(0)
#else
# define ENTER(...)	do{}while(0)
# define LOG(...)	do{}while(0)
# define LEAVE(...)	do{}while(0)
# define LEAVE_RET(t,v)	do{return v;}while(0)
#endif

// Threads
extern void	**Threads_GetHandlesPtr(void);
extern int	*Threads_GetErrno(void);
//extern tPGID	Threads_GetPGID(void);
//extern tPID	Threads_GetPID(void);
extern tTID	Threads_GetTID(void);
extern tUID	Threads_GetUID(void);
extern tGID	Threads_GetGID(void);

extern struct sThread	*Proc_SpawnWorker(void (*Fcn)(void*), void *Data);
extern void	Threads_Sleep(void);
extern void	Threads_Yield(void);
extern int	Threads_SetName(const char *NewName);

// Kinda hacky way of not colliding with native errno
#define errno	(*(Threads_GetErrno()))

/**
 * \name Endianness Swapping
 * \{
 */
#ifdef __BIG_ENDIAN__
#define	LittleEndian16(_val)	SwapEndian16(_val)
#define	LittleEndian32(_val)	SwapEndian32(_val)
#define	LittleEndian64(_val)	SwapEndian32(_val)
#define	BigEndian16(_val)	(_val)
#define	BigEndian32(_val)	(_val)
#define	BigEndian64(_val)	(_val)
#else
#define	LittleEndian16(_val)	(_val)
#define	LittleEndian32(_val)	(_val)
#define	LittleEndian64(_val)	(_val)
#define	BigEndian16(_val)	SwapEndian16(_val)
#define	BigEndian32(_val)	SwapEndian32(_val)
#define	BigEndian64(_val)	SwapEndian64(_val)
#endif
extern Uint16	SwapEndian16(Uint16 Val);
extern Uint32	SwapEndian32(Uint32 Val);
extern Uint64	SwapEndian64(Uint64 Val);
/**
 * \}
 */


#include <string.h>
#include <acess_string.h>
#if 0
extern int	strucmp(const char *s1, const char *s2);
extern int	strpos(const char *Str, char Ch);
extern void	itoa(char *buf, uint64_t num, int base, int minLength, char pad);
extern int	snprintf(char *buf, size_t len, const char *fmt, ...);
extern int	sprintf(char *buf, const char *fmt, ...);
extern int	ReadUTF8(const Uint8 *str, Uint32 *Val);
extern int	WriteUTF8(Uint8 *str, Uint32 Val);
extern int	ParseInt(const char *string, int *Val);
#include <ctype.h>
extern int	ModUtil_SetIdent(char *Dest, const char *Value);
extern int	ModUtil_LookupString(const char **Array, const char *Needle);
extern Uint8	ByteSum(const void *Ptr, int Size);
extern int	Hex(char *Dest, size_t Size, const Uint8 *SourceData);
extern int	UnHex(Uint8 *Dest, size_t DestSize, const char *SourceString);
#endif
#define CheckString(str)	(1)
#define CheckMem(mem,sz)	(1)
extern int	rand(void);

// TODO: Move out?
extern int	DivUp(int value, int divisor);
extern uint64_t	DivMod64U(uint64_t Num, uint64_t Den, uint64_t *Rem);
static inline int MIN(int a, int b) { return a < b ? a : b; }
static inline int MAX(int a, int b) { return a > b ? a : b; }

#include <shortlock.h>

static inline intptr_t MM_GetPhysAddr(void *Ptr) { return 1; }
static inline int	MM_IsUser(const void *Ptr) { return 1; }

#endif

