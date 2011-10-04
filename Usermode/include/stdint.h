/*
 */
#ifndef _STDTYPES_H_
#define _STDTYPES_H_

//typedef unsigned int	uint;
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned int	uint32_t;
typedef unsigned long long	uint64_t;

typedef signed char		int8_t;
typedef signed short	int16_t;
typedef signed int		int32_t;
typedef signed long long	int64_t;

#ifdef __LP64__
typedef uint64_t	intptr_t;
typedef uint64_t	uintptr_t;
typedef int64_t	ptrdiff_t;
#else
typedef uint32_t	intptr_t;
typedef uint32_t	uintptr_t;
typedef int32_t	ptrdiff_t;
#endif
#if 0
# error "Unknown pointer size"
#endif

typedef uint64_t	off_t;

#endif
