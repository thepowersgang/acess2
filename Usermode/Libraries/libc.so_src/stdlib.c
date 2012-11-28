/*
 * AcessOS Basic C Library
 * stdlib.c
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
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

EXPORT unsigned long long strtoull(const char *str, char **end, int base)
{
	long long	ret = 0;
	
	if( !str || base < 0 || base > 36 || base == 1 ) {
		if(end)
			*end = (char*)str;
		errno = EINVAL;
		return 0;
	}

	while( isspace(*str) )
		str++;
	
	if( base == 0 || base == 16 ) {
		if( *str == '0' && str[1] == 'x' ) {
			str += 2;
			base = 16;
		}
	}
	
	if( base == 0 && *str == '0' ) {
		str ++;
		base = 8;
	}

	if( base == 0 )
		base = 10;

	while( *str )
	{
		 int	next = -1;
		if( base <= 10 ) {
			if( '0' <= *str && *str <= '0'+base-1 )
				next = *str - '0';
		}
		else {
			if( '0' <= *str && *str <= '9' )
				next = *str - '0';
			if( 'A' <= *str && *str <= 'A'+base-10-1 )
				next = *str - 'A';
			if( 'a' <= *str && *str <= 'a'+base-10-1 )
				next = *str - 'a';
		}
		if( next < 0 )
			break;
		ret *= base;
		ret += next;
		str ++;
	}

	if(end)
		*end = (char*)str;
	return ret;
}

EXPORT unsigned long strtoul(const char *ptr, char **end, int base)
{
	unsigned long long tmp = strtoull(ptr, end, base);
	
	if( tmp > ULONG_MAX ) {
		errno = ERANGE;
		return ULONG_MAX;
	}
	
	return tmp;
}

EXPORT long long strtoll(const char *str, char **end, int base)
{
	 int	neg = 0;
	unsigned long long	ret;

	if( !str ) {
		errno = EINVAL;
		return 0;
	}
	
	while( isspace(*str) )
		str++;
	
	// Check for negative (or positive) sign
	if(*str == '-' || *str == '+') {
		neg = (*str == '-');
		str++;
	}

	ret = strtoull(str, end, base);	

	if( neg )
		return -ret;
	else
		return ret;
}

EXPORT long strtol(const char *str, char **end, int base)
{
	long long tmp = strtoll(str, end, base);
	if( tmp > LONG_MAX || tmp < LONG_MIN ) {
		errno = ERANGE;
		return (tmp > LONG_MAX) ? LONG_MAX : LONG_MIN;
	}
	return tmp;
}

/**
 * \fn EXPORT int atoi(const char *str)
 * \brief Convert a string to an integer
 */
EXPORT int atoi(const char *str)
{
	long long	tmp = strtoll(str, NULL, 0);
	if( tmp > INT_MAX || tmp < INT_MIN ) {
		errno = ERANGE;
		return (tmp > INT_MAX) ? INT_MAX : INT_MIN;
	}
	return tmp;
}

int abs(int j) { return j < 0 ? -j : j; }
long int labs(long int j) { return j < 0 ? -j : j; }

