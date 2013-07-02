/*
 * Acess2 Window Manager v3 (axwin3)
 * - By John Hodge (thePowersGang)
 *
 * include/common.h
 * - Common definitions and functions
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#ifndef AXWIN_SDL_BUILD
#include <acess/sys.h>
#else
#include <stdio.h>
#define _SysDebug(f,a...)	fprintf(stderr, f"\n" ,## a)
#endif

#define TODO(str)	

#define ASSERT(expr)	do{if(!(expr)){_SysDebug("%s:%i - ASSERTION FAILED: "#expr, __FILE__, __LINE__);exit(-1);}}while(0)

#define UNIMPLEMENTED()	do{_SysDebug("TODO: Implement %s", __func__); for(;;);}while(0)

#define	AXWIN_VERSION	0x300

static inline int MIN(int a, int b)	{ return (a < b) ? a : b; }
static inline int MAX(int a, int b)	{ return (a > b) ? a : b; }

extern int	giScreenWidth, giScreenHeight;

#endif

