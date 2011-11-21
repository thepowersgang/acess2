/*
 */
#ifndef _STDINT_H_
#define _STDINT_H_

#define INT_MIN	-0x80000000
#define INT_MAX	0x7FFFFFFF

typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned int	uint32_t;
typedef unsigned long long	uint64_t;

typedef signed char		int8_t;
typedef signed short	int16_t;
typedef signed int		int32_t;
typedef signed long long	int64_t;

#if ARCHDIR_is_x86
typedef int32_t 	intptr_t;
typedef uint32_t	uintptr_t;
#elif ARCHDIR_is_x86_64
typedef int64_t 	intptr_t;
typedef uint64_t	uintptr_t;
#elif ARCHDIR_is_armv7
typedef int32_t 	intptr_t;
typedef uint32_t 	uintptr_t;
#else
# error "Unknown pointer size"
#endif

//typedef uint64_t	off_t;

#endif
