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
 * \fn EXPORT char *strncpy(char *dst, const char *src)
 * \brief Copy at most \a num characters from \a src to \a dst
 * \return \a dst
 */
EXPORT char *strncpy(char *dst, const char *src, size_t num)
{
	char *to = dst;
	while(*src && num--)	*to++ = *src++;
	*to = '\0';
	return dst;
}

/**
 * \fn EXPORT char *strcat(char *dst, const char *src)
 * \brief Append a string onto another
 */
EXPORT char *strcat(char *dst, const char *src)
{
	char	*to = dst;
	// Find the end
	while(*to)	to++;
	// Copy
	while(*src)	*to++ = *src++;
	// End string
	*to = '\0';
	return dst;
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

/**
 * \fn EXPORT char *strndup(const char *str, size_t maxlen)
 * \brief Duplicate a string into the heap with a maximum length
 * \param str	Input string buffer
 * \param maxlen	Maximum valid size of the \a str buffer
 * \return Heap string with the same value of \a str
 */
EXPORT char *strndup(const char *str, size_t maxlen)
{
	size_t	len;
	char	*ret;
	for( len = 0; len < maxlen && str[len]; len ++) ;
	ret = malloc( len + 1);
	memcpy( ret, str, len );
	ret[len] = '\0';
	return ret;
}

/**
 * \fn EXPORT char *strchr(char *str, int character)
 * \brief Locate a character in a string
 */
EXPORT char *strchr(char *str, int character)
{
	while(*str)
	{
		if(*str == character)	return str;
	}
	return NULL;
}

/**
 * \fn EXPORT char *strrchr(char *str, int character)
 * \brief Locate the last occurance of a character in a string
 */
EXPORT char *strrchr(char *str, int character)
{
	 int	i;
	i = strlen(str)-1;
	while(i--)
	{
		if(str[i] == character)	return &str[i];
	}
	return NULL;
}

/**
 * \fn EXPORT char *strstr(char *str1, const char *str2)
 * \brief Search a \a str1 for the first occurance of \a str2
 */
EXPORT char *strstr(char *str1, const char *str2)
{
	const char	*test = str2;
	
	while(*str1)
	{
		if(*test == '\0')	return str1;
		if(*str1 == *test)	test++;
		else	test = str2;
		str1 ++;
	}
	return NULL;
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

/**
 * \fn EXPORT int memcmp(const void *mem1, const void *mem2, size_t count)
 * \brief Compare two regions of memory
 * \param mem1	Region 1
 * \param mem2	Region 2
 * \param count	Number of bytes to check
 */
EXPORT int memcmp(const void *mem1, const void *mem2, size_t count)
{
	while(count--)
	{
		if( *(unsigned char*)mem1 != *(unsigned char*)mem2 )
			return *(unsigned char*)mem1 - *(unsigned char*)mem2;
		mem1 ++;
		mem2 ++;
	}
	return 0;
}

/**
 * \fn EXPORT void *memchr(void *ptr, int value, size_t num)
 * \brief Locates the first occurence of \a value starting at \a ptr
 * \param ptr	Starting memory location
 * \param value	Value to find
 * \param num	Size of memory area to check
 */
EXPORT void *memchr(void *ptr, int value, size_t num)
{
	while(num--)
	{
		if( *(unsigned char*)ptr == (unsigned char)value )
			return ptr;
		ptr ++;
	}
	return NULL;
}
