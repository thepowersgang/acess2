/*
AcessOS Basic Lib C

lib.h
*/
#ifndef _LIB_H
#define _LIB_H

#define BUILD_SO	1

#if defined(BUILD_DLL)
#define	EXPORT	__declspec(dllexport)
#define	LOCAL
#elif defined(BUILD_SO)
#define	EXPORT
#define	LOCAL
#endif

extern void *memcpy(void *dest, const void *src, size_t n);

#endif
