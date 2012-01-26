/*
 */
#ifndef _STDINT_H_
#define _STDINT_H_

#include "acess/intdefs.h"

#define INT_MIN	-0x80000000
#define INT_MAX	0x7FFFFFFF

typedef __uint8_t	uint8_t;
typedef __uint16_t	uint16_t;
typedef __uint32_t	uint32_t;
typedef __uint64_t	uint64_t;
typedef __int8_t	int8_t;
typedef __int16_t	int16_t;
typedef __int32_t	int32_t;
typedef __int64_t	int64_t;

typedef __intptr_t	intptr_t;
typedef __uintptr_t	uintptr_t;

//typedef uint64_t	off_t;

#endif
