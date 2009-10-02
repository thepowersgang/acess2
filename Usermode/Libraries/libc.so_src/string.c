/*
 * AcessOS Basic C Library
 * string.c
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include "lib.h"

/**
 * \fn EXPORT int strcmp(const char *s1, const char *s2)
 * \brief Compare two strings
 */
EXPORT int strcmp(const char *s1, const char *s2)
{
	while(*s1 == *s2 && *s1 != '\0' && *s2 != '\0') {
		s1++; s2++;
	}
	return (int)*s1 - (int)*s2;
}

/**
 * \fn EXPORT char *strcpy(char *dst, const char *src)
 * \brief Copy a string to another
 */
EXPORT char *strcpy(char *dst, const char *src)
{
	char *_dst = dst;
	while(*src) {
		*dst = *src;
		src++; dst++;
	}
	*dst = '\0';
	return _dst;
}

/**
 * \fn EXPORT int strlen(const char *str)
 * \brief Get the length of a string
 */
EXPORT int strlen(const char *str)
{
	int retval;
	for(retval = 0; *str != '\0'; str++)
		retval++;
	return retval;
}

/**
 * \fn EXPORT int strncmp(const char *s1, const char *s2, size_t len)
 * \brief Compare two strings with a limit
 */
EXPORT int strncmp(const char *s1, const char *s2, size_t len)
{
	while(--len && *s1 == *s2 && *s1 != '\0' && *s2 != '\0') {
		s1++; s2++;
	}
	return (int)*s1 - (int)*s2;
}

/**
 * \fn EXPORT char *strdup(const char *str)
 * \brief Duplicate a string using heap memory
 * \note Defined in POSIX Spec, not C spec
 */
EXPORT char *strdup(const char *str)
{
	size_t	len = strlen(str);
	char	*ret = malloc(len+1);
	if(ret == NULL)	return NULL;
	strcpy(ret, str);
	return ret;
}

// --- Memory ---
/**
 * \fn EXPORT void *memset(void *dest, int val, size_t num)
 * \brief Clear memory with the specified value
 */
EXPORT void *memset(void *dest, int val, size_t num)
{
	unsigned char *p = dest;
	while(num--)	*p++ = val;
	return dest;
}

/**
 * \fn EXPORT void *memcpy(void *dest, const void *src, size_t count)
 * \brief Copy one memory area to another
 */
EXPORT void *memcpy(void *dest, const void *src, size_t count)
{
    char *sp = (char *)src;
    char *dp = (char *)dest;
    for(;count--;) *dp++ = *sp++;
    return dest;
}

/**
 * \fn EXPORT void *memmove(void *dest, const void *src, size_t count)
 * \brief Copy data in memory, avoiding overlap problems
 */
EXPORT void *memmove(void *dest, const void *src, size_t count)
{
    char *sp = (char *)src;
    char *dp = (char *)dest;
	// Check if corruption will happen
	if( (unsigned int)dest > (unsigned int)src && (unsigned int)dest < (unsigned int)src+count )
		for(;count--;) dp[count] = sp[count];
	else
    	for(;count--;) *dp++ = *sp++;
    return dest;
}
