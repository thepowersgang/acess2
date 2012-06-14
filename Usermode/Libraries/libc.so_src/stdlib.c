/*
 * AcessOS Basic C Library
 * stdlib.c
 */
/**
 * \todo Move half of these to stdio
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include "lib.h"

#define _stdout	1
#define _stdin	0

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
	 int	i, j, min;
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

/**
 * \fn EXPORT int atoi(const char *str)
 * \brief Convert a string to an integer
 */
EXPORT int atoi(const char *str)
{
	 int	neg = 0;
	 int	ret = 0;
	
	// NULL Check
	if(!str)	return 0;
	
	while(*str == ' ' || *str == '\t')	str++;
	
	// Check for negative
	if(*str == '-') {
		neg = 1;
		str++;
	}
	
	if(*str == '0') {
		str ++;
		if(*str == 'x') {
			str++;
			// Hex
			while( ('0' <= *str && *str <= '9')
				|| ('A' <= *str && *str <= 'F' )
				|| ('a' <= *str && *str <= 'f' )
				)
			{
				ret *= 16;
				if(*str <= '9') {
					ret += *str - '0';
				} else if (*str <= 'F') {
					ret += *str - 'A' + 10;
				} else {
					ret += *str - 'a' + 10;
				}
				str++;
			}
		} else {
			// Octal
			while( '0' <= *str && *str <= '7' )
			{
				ret *= 8;
				ret += *str - '0';
				str++;
			}
		}
	} else {
		// Decimal
		while( '0' <= *str && *str <= '9' )
		{
			ret *= 10;
			ret += *str - '0';
			str++;
		}
	}
	
	// Negate if needed
	if(neg)	ret = -ret;
	return ret;
}

int abs(int j) { return j < 0 ? -j : j; }
long int labs(long int j) { return j < 0 ? -j : j; }

