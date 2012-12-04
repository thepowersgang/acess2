/*
 * AcessOS Basic C Library
 * stdlib.c
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include "lib.h"

extern void	*_crt0_exit_handler;

// === PROTOTYPES ===
EXPORT int	atoi(const char *str);
EXPORT void	exit(int status);

// === GLOBALS ===
void	(*g_stdlib_exithandler)(void);

// === CODE ===
void atexit(void (*__func)(void))
{
	g_stdlib_exithandler = __func;
	// TODO: Replace with meta-function to allow multiple atexit() handlers
	_crt0_exit_handler = __func;	
}

/**
 * \fn EXPORT void exit(int status)
 * \brief Exit
 */
EXPORT void exit(int status)
{
	if( g_stdlib_exithandler )
		g_stdlib_exithandler();
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

