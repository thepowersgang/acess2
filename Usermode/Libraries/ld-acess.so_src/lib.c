/*
 AcessOS 1
 Dynamic Loader
 By thePowersGang
*/
#include "common.h"

// === CODE ===
void strcpy(char *dest, char *src)
{
	while(*src) {
		*dest = *src;
		src ++;	dest ++;
	}
	*dest = '\0';
}

int strcmp(char *s1, char *s2)
{
	while(*s1 == *s2 && *s1 != 0) s1++,s2++;
	return *s1-*s2;
}

int strlen(char *str)
{
	 int	len = 0;
	while(*str)	len++,str++;
	return len;
}

