/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * strtoi.c
 * - strto[u][l]l/atoi implimentation
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>

unsigned long long strtoull(const char *str, char **end, int base)
{
	long long	ret = 0;
	
	if( !str || base < 0 || base > 36 || base == 1 ) {
		if(end)
			*end = (char*)str;
		errno = EINVAL;
		return 0;
	}

	// Trim leading spaces
	while( isspace(*str) )
		str++;
	
	// Handle base detection for hex
	if( base == 0 || base == 16 ) {
		if( *str == '0' && str[1] == 'x' ) {
			str += 2;
			base = 16;
		}
	}
	
	// Handle base detection for octal
	if( base == 0 && *str == '0' ) {
		str ++;
		base = 8;
	}

	// Fall back on decimal when unknown
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

unsigned long strtoul(const char *ptr, char **end, int base)
{
	unsigned long long tmp = strtoull(ptr, end, base);
	
	if( tmp > ULONG_MAX ) {
		errno = ERANGE;
		return ULONG_MAX;
	}
	
	return tmp;
}

long long strtoll(const char *str, char **end, int base)
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

long strtol(const char *str, char **end, int base)
{
	long long tmp = strtoll(str, end, base);
	if( tmp > LONG_MAX || tmp < LONG_MIN ) {
		errno = ERANGE;
		return (tmp > LONG_MAX) ? LONG_MAX : LONG_MIN;
	}
	return tmp;
}

/**
 * \fn int atoi(const char *str)
 * \brief Convert a string to an integer
 */
int atoi(const char *str)
{
	long long	tmp = strtoll(str, NULL, 0);
	if( tmp > INT_MAX || tmp < INT_MIN ) {
		errno = ERANGE;
		return (tmp > INT_MAX) ? INT_MAX : INT_MIN;
	}
	return tmp;
}

long atol(const char *str)
{
	long long	tmp = strtoll(str, NULL, 0);
	if( tmp > LONG_MAX || tmp < LONG_MIN ) {
		errno = ERANGE;
		return (tmp > LONG_MAX) ? LONG_MAX : LONG_MIN;
	}
	return tmp;
}

long long atoll(const char *str)
{
	long long	tmp = strtoll(str, NULL, 0);
	return tmp;
}
