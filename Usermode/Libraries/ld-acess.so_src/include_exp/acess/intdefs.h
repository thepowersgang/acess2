/*
 */
#ifndef _ACESS_INTDEFS_H_
#define _ACESS_INTDEFS_H_

#define INT_MIN	-0x80000000
#define INT_MAX	0x7FFFFFFF

typedef unsigned char	__uint8_t;
typedef unsigned short	__uint16_t;
typedef unsigned int	__uint32_t;
typedef unsigned long long	__uint64_t;

typedef signed char		__int8_t;
typedef signed short	__int16_t;
typedef signed int		__int32_t;
typedef signed long long	__int64_t;

#if defined(ARCHDIR_is_x86)
typedef __int32_t 	__intptr_t;
typedef __uint32_t	__uintptr_t;
#elif defined(ARCHDIR_is_x86_64)
typedef __int64_t 	__intptr_t;
typedef __uint64_t	__uintptr_t;
#elif defined(ARCHDIR_is_armv7) | defined(ARCHDIR_is_armv6)
typedef __int32_t 	__intptr_t;
typedef __uint32_t 	__uintptr_t;
#else
# error "Unknown pointer size"
#endif

//typedef uint64_t	off_t;

#endif

