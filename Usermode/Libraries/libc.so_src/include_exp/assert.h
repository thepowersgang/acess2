/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * assert.h
 * - assert(expr)
 */
#ifndef _LIBC__ASSERT_H_
#define _LIBC__ASSERT_H_

#include <stdlib.h>
//#include <stdio.h>
#include <acess/sys.h>
//extern void exit(int status);

#ifdef NDEBUG
# define assert(expr)	do{}while(0)
#else
//# define assert(expr)	do{if(!(expr)) { fprintf(stderr, "%s:%i: Assertion '%s' failed\n", __FILE__, __LINE__, #expr); exit(-1);}}while(0)
# define assert(expr)	do{if(!(expr)) { _SysDebug("%s:%i: Assertion '%s' failed", __FILE__, __LINE__, #expr); exit(-1);}}while(0)
#endif

#endif

