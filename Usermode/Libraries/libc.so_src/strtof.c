/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * strtof.c
 * - strto[df]/atof implimentation
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>	// strtoll

double strtod(const char *ptr, char **end)
{
	 int	negative = 0;
	 int	is_hex = 0;
	double	rv = 0;
	char	*tmp;
	
	while( isspace(*ptr) )
		ptr ++;
	
	if( *ptr == '+' || *ptr == '-' )
	{
		negative = (*ptr == '-');
		ptr ++;
	}
		
	#if 0
	// NAN.*
	if( strncasecmp(ptr, "NAN", 3) == 0)
	{
		ptr += 3;
		while( isalnum(*ptr) || *ptr == '_' )
			ptr ++;
		
		rv = negative ? -NAN : NAN;
		if(end)	*end = ptr;
		return rv;
	}

	// INF/INFINITY
	if( strncasecmp(ptr, "INF", 3) == 0 )
	{
		if( strncasecmp(ptr, "INFINITY", 8) == 0 )
			ptr += 8;
		else
			ptr += 3;
		rv = negative ? -INFINITY : INFINITY;
		if(end)	*end = ptr;
		return rv;
	}
	#endif

	if( *ptr == '0' && (ptr[1] == 'X' || ptr[1] == 'x') )
	{
		is_hex = 1;
		ptr += 2;
	}
	
	rv = strtoll(ptr, &tmp, (is_hex ? 16 : 10));
	
	// TODO: errors?
	
	ptr = tmp;
	if( *ptr == '.' )
	{
		ptr ++;
		if( isspace(*ptr) || *ptr == '-' || *ptr == '+' ) {
			goto _err;
		}
		double frac = strtoll(ptr, &tmp, (is_hex ? 16 : 10));
		// TODO: Error check
		while( ptr != tmp )
		{
			frac /= 10;
			ptr ++;
		}
		rv += frac;
	}
	
	 int	exp = 0;
	if( is_hex )
	{
		if( *ptr == 'P' || *ptr == 'p' )
		{
			ptr ++;
			exp = strtoll(ptr, &tmp, 16);
			ptr = tmp;
		}
	}
	else
	{
		if( *ptr == 'E' || *ptr == 'e' )
		{
			ptr ++;
			exp = strtoll(ptr, &tmp, 10);
			ptr = tmp;
		}
	}

	if( exp == 0 )
		;
	else if( exp < 0 )
		while( exp ++ )
			rv /= 10;
	else
		while( exp -- )
			rv *= 10;

	if( negative )
		rv = -rv;
	
	if( end )
		*end = (char*)ptr;
	
	return rv;
_err:
	if( end )
		*end = (char*)ptr;
	
	return 0.0f;
}

float strtof(const char *ptr, char **end)
{
	double rv = strtod(ptr, end);
	// TODO: ERANGE
	return rv;
}

float atof(const char *ptr)
{
	return strtof(ptr, NULL);
}
