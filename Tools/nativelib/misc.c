/*
 * Acess2 nativelib
 * - By John Hodge (thePowersGang)
 * 
 * misc.c
 * - Random functions
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>	// strcasecmp

// TODO: Move into a helper lib?
void itoa(char *buf, uint64_t num, int base, int minLength, char pad)
{
	char fmt[] = "%0ll*x";
	switch(base)
	{
	case  8:	fmt[5] = 'o';	break;
	case 10:	fmt[5] = 'd';	break;
	case 16:	fmt[5] = 'x';	break;
	}
	if(pad != '0') {
		fmt[1] = '%';
		sprintf(buf, fmt+1, minLength, num);
	}
	else {
		sprintf(buf, fmt, minLength, num);
	}
}

int ParseInt(const char *string, int *value)
{
	char *next;
	*value = strtol(string, &next, 0);
	return next - string;
}

int strpos(const char *Str, char Ch)
{
	const char *r = strchr(Str, Ch);
	if(!r)	return -1;
	return r - Str;
}

int strucmp(const char *s1, const char *s2)
{
	return strcasecmp(s1, s2);
}

uint64_t DivMod64U(uint64_t value, uint64_t divisor, uint64_t *remainder)
{
	if(remainder)
		*remainder = value % divisor;
	return value / divisor;
}
