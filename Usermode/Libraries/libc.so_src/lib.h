/*
 * Acess2 Basic Lib C
 * lib.h
*/
#ifndef _LIB_H
#define _LIB_H

#include <stddef.h>
#include <stdint.h>

#define BUILD_SO	1

#if defined(BUILD_DLL)
#define	EXPORT	__declspec(dllexport)
#define	LOCAL
#elif defined(BUILD_SO)
#define	EXPORT
#define	LOCAL
#endif

#define UNUSED(type, param)	__attribute__((unused)) type UNUSED__##param

extern void *memcpy(void *dest, const void *src, size_t n);

typedef struct sCPUID	tCPUID;

struct sCPUID
{
	uint8_t	SSE;
	uint8_t	SSE2;
};

#endif
