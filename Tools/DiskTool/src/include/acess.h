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

#define BITS	32
#define NULL	((void*)0)
#include <stdint.h>

typedef uintptr_t	Uint;
typedef unsigned int	size_t;
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

// NOTE: Since this is single-threaded (for now) mutexes can be implimented as simple locks
typedef char	tMutex;
typedef char	tShortSpinlock;

typedef int64_t	tTime;

#define PACKED	__attribute__((packed))
#define DEPRECATED
#define EXPORT(s)
#define EXPORTV(s)

#include <vfs_ext.h>

// These are actually library functions, but they can't be included, so they're defined manually
extern void	*malloc(size_t bytes);
extern void	*calloc(size_t nmemb, size_t size);
extern void	*realloc(void *oldptr, size_t bytes);
extern void	free(void *buffer);

#include <errno.h>

#include <acess_logging.h>

// Threads
extern int	*Threads_GetErrno(void);
//extern tPGID	Threads_GetPGID(void);
//extern tPID	Threads_GetPID(void);
extern tTID	Threads_GetTID(void);
extern tUID	Threads_GetUID(void);
extern tGID	Threads_GetGID(void);

// Kinda hacky way of not colliding with native errno
#define errno	(*(Threads_GetErrno()))

#include <string.h>
extern int	strpos(const char *Str, char Ch);
extern void	itoa(char *buf, uint64_t num, int base, int minLength, char pad);

// TODO: Move out?
extern int64_t	DivUp(int64_t value, int64_t divisor);

#if DEBUG
# define ENTER(str, v...)	Log("%s:%i: ENTER "str, __func__, __LINE__)
# define LOG(fmt, v...) 	Log("%s:%i: "fmt, __func__, __LINE__, ##v)
# define LEAVE(...)	do{}while(0)
# define LEAVE_RET(t,v)	return v;
#else
# define ENTER(...)	do{}while(0)
# define LOG(...)	do{}while(0)
# define LEAVE(...)	do{}while(0)
# define LEAVE_RET(t,v)	return v;
#endif

static inline int Mutex_Acquire(tMutex *m) {
	if(*m)	Log_KernelPanic("---", "Double mutex lock");
	*m = 1;
	return 0;
}
static inline void Mutex_Release(tMutex *m) { *m = 0; }

static inline void SHORTLOCK(tShortSpinlock *Lock) {
	if(*Lock)	Log_KernelPanic("---", "Double short lock");
	*Lock = 1;
}
static inline void SHORTREL(tShortSpinlock *m) { *m = 0; }

#endif

