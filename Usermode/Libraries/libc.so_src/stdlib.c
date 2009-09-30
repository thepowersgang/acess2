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

// === IMPORTS ===
extern int	fprintfv(FILE *fp, const char *format, va_list args);

// === PROTOTYPES ===
EXPORT int	atoi(const char *str);
EXPORT void	exit(int status);

// === CODE ===
/**
 * \fn EXPORT void exit(int status)
 * \brief Exit
 */
EXPORT void exit(int status)
{
	_exit(status);
}

/**
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
