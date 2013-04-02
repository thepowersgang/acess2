/*
 * AcessOS Basic C Library
 * stdlib.c
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include "lib.h"

extern void	_stdio_cleanup(void);

#define MAX_ATEXIT_HANDLERS	64	// Standard defines >=32

// === PROTOTYPES ===
EXPORT int	atoi(const char *str);
EXPORT void	exit(int status);

// === GLOBALS ===
typedef void (*stdlib_exithandler_t)(void);
stdlib_exithandler_t	g_stdlib_exithandlers[MAX_ATEXIT_HANDLERS];
 int	g_stdlib_num_exithandlers;

// === CODE ===
void _call_atexit_handlers(void)
{
	 int	i;
	for( i = g_stdlib_num_exithandlers; i --; )
		g_stdlib_exithandlers[i]();
	_stdio_cleanup();
}

EXPORT void atexit(void (*__func)(void))
{
	if( g_stdlib_num_exithandlers < MAX_ATEXIT_HANDLERS )
	{
		g_stdlib_exithandlers[g_stdlib_num_exithandlers++] = __func;
	}
}

/**
 * \fn EXPORT void exit(int status)
 * \brief Exit
 */
EXPORT void exit(int status)
{
	_call_atexit_handlers();
	_exit(status);
}

/**
 * \fn EXPORT void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *))
 * \brief Sort an array
 * \note Uses a selection sort
 */
EXPORT void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *))
{
	size_t	i, j, min;
	// With 0 items, there's nothing to do and with 1 its already sorted
	if(nmemb == 0 || nmemb == 1)	return;
	
	// SORT!
	for( i = 0; i < (nmemb-1); i++ )
	{
		min = i;
		for( j = i+1; j < nmemb; j++ )
		{
			if(compar(base+size*j, base + size*min) < 0) {
				min = j;
			}
		}
		if (i != min) {
			char	swap[size];
			memcpy(swap, base+size*i, size);
			memcpy(base+size*i, base+size*min, size);
			memcpy(base+size*i, swap, size);
		}
	}
}


int abs(int j) { return j < 0 ? -j : j; }
long int labs(long int j) { return j < 0 ? -j : j; }

