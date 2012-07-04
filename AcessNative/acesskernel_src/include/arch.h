/**
 */
#ifndef _ARCH_H_
#define _ARCH_H_

#include <stdint.h>
//#include <stdlib.h>
#include <pthread.h>
#undef CLONE_VM
#define	_MODULE_NAME_	"NativeKernel"

#define PAGE_SIZE	0x1000	// Assume, making an Ass out of u and me
#define BITS	(sizeof(intptr_t)*8)

typedef uint8_t	Uint8;
typedef uint16_t	Uint16;
typedef uint32_t	Uint32;
typedef uint64_t	Uint64;

typedef int8_t	Sint8;
typedef int16_t	Sint16;
typedef int32_t	Sint32;
typedef int64_t	Sint64;

typedef intptr_t	Uint;

typedef intptr_t	tVAddr;
typedef intptr_t	tPAddr;

typedef	int	BOOL;

struct sShortSpinlock
{
	 int	IsValid;
	pthread_mutex_t	Mutex;
};

#define SHORTLOCK(...)
#define SHORTREL(...)

//#define	NUM_CFG_ENTRIES	10

extern void	Debug_PutCharDebug(char ch);
extern void	Debug_PutStringDebug(const char *str);

#endif

