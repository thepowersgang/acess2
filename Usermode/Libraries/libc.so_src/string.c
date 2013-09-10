/*
 * AcessOS Basic C Library
 * string.c
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "lib.h"

/**
 * \fn EXPORT int strcmp(const char *s1, const char *s2)
 * \brief Compare two strings
 */
EXPORT int strcmp(const char *s1, const char *s2)
{
	while(*s1 && *s1 == *s2) {
		s1++; s2++;
	}
	return (int)*s1 - (int)*s2;
}

/**
 * \fn EXPORT int strncmp(const char *s1, const char *s2)
 * \brief Compare two strings
 */
EXPORT int strncmp(const char *s1, const char *s2, size_t n)
{
	while(n && *s1 && *s1 == *s2)
	{
		s1++; s2++;
		n --;
	}
	if( n == 0 )
		return 0;
	else
		return (int)*s1 - (int)*s2;
}

EXPORT int strcasecmp(const char *s1, const char *s2)
{
	 int	rv;
	while( (rv = toupper(*s1) - toupper(*s2)) == 0 && *s1 != '\0' && *s2 != '\0' ) {
		s1++; s2++;
	}
	return rv;
}

EXPORT int strncasecmp(const char *s1, const char *s2, size_t n)
{
	 int	rv = 0;
	if( n == 0 )	return 0;
	while(n -- && (rv = toupper(*s1) - toupper(*s2)) == 0 && *s1 != '\0' && *s2 != '\0') {
		s1++; s2++;
	}
	return rv;
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

EXPORT char *strncat(char *dst, const char *src, size_t n)
{
	char	*to = dst;
	// Find the end
	while(*to)	to++;
	// Copy
	while(*src && n--)	*to++ = *src++;
	// End string
	*to = '\0';
	return dst;
}

/**
 * \brief Get the length of a string
 */
EXPORT size_t strlen(const char *str)
{
	size_t	retval;
	for(retval = 0; *str != '\0'; str++, retval++);
	return retval;
}

/**
 * \brief Get the length of a string, with a maximum of \a maxlen
 * 
 * Gets the length of a string (excluding the terminating \0 byte)
 */
EXPORT size_t strnlen(const char *str, size_t maxlen)
{
	size_t	len;
	for( len = 0; maxlen -- && *str; str ++, len ++ );
	return len;
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
EXPORT char *strchr(const char *str, int character)
{
	for(;*str;str++)
	{
		if(*str == character)
			return (char*)str;
	}
	return NULL;
}

/**
 * \fn EXPORT char *strrchr(char *str, int character)
 * \brief Locate the last occurance of a character in a string
 */
EXPORT char *strrchr(const char *str, int character)
{
	 int	i;
	i = strlen(str)-1;
	while(i--)
	{
		if(str[i] == character)
			return (void*)&str[i];
	}
	return NULL;
}

/**
 * \fn EXPORT char *strstr(char *str1, const char *str2)
 * \brief Search a \a str1 for the first occurance of \a str2
 */
EXPORT char *strstr(const char *str1, const char *str2)
{
	const char	*test = str2;
	
	for(;*str1;str1++)
	{
		if(*test == '\0')	return (char*)str1;
		if(*str1 == *test)	test++;
		else	test = str2;
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
EXPORT void *memcpy(void *__dest, const void *__src, size_t count)
{
	const int	wordmask = sizeof(void*)-1;
	uintptr_t	src = (uintptr_t)__src;
	uintptr_t	dst = (uintptr_t)__dest;

	if( count < sizeof(void*)*2 || (dst & wordmask) != (src & wordmask) )
	{
		char	*dp = __dest;
		const char	*sp = __src;
		while(count--) *dp++ = *sp ++;
	}
	// TODO: Bulk aligned copies
	#if 0
	else if(count > 128 && (dst & 15) == (src & 15) )
	{
		// SSE/bulk copy
		for( ; dst & 15; count -- )
			*(char*)dst++ = *(char*)src++;
		memcpy_16byte(dst, src, count / 16);
		dst += count & ~15;
		src += count & ~15;
		count &= 15;
		while(count --)
			*(char*)dst++ = *(char*)src++;
	}
	#endif
	else
	{
		void	**dp, **sp;
		for( ; count && (dst & wordmask) != 0; count -- )
			*(char*)dst++ = *(char*)src++;

		dp = (void*)dst; sp = (void*)src;
		while( count >= sizeof(void*) )
		{
			*dp++ = *sp++;
			count -= sizeof(void*);
		}
		dst = (uintptr_t)dp; src = (uintptr_t)sp;
		for( ; count; count -- )
			*(char*)dst++ = *(char*)src++;
	}

	return __dest;
}

/**
 * \fn EXPORT void *memmove(void *dest, const void *src, size_t count)
 * \brief Copy data in memory, avoiding overlap problems
 */
EXPORT void *memmove(void *dest, const void *src, size_t count)
{
	const char *sp = (const char *)src;
	char *dp = (char *)dest;
	// Check if the areas overlap
	if( sp >= dp+count )
		memcpy(dest, src, count);
	else if( dp >= sp+count )
		memcpy(dest, src, count);
	else {
		if( sp < dp ) {
			while(count--)
				dp[count] = sp[count];
		}
		else {
			while(count--)
				*dp++ = *sp++;
		}
	}
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
	const unsigned char	*p1 = mem1, *p2 = mem2;
	while(count--)
	{
		if( *p1 != *p2 )
			return *p1 - *p2;
		p1 ++;
		p2 ++;
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
EXPORT void *memchr(const void *ptr, int value, size_t num)
{
	while(num--)
	{
		if( *(const unsigned char*)ptr == (unsigned char)value )
			return (void*)ptr;
		ptr ++;
	}
	return NULL;
}

EXPORT size_t strcspn(const char *haystack, const char *reject)
{
	size_t	ret = 0;
	 int	i;
	while( *haystack )
	{
		for( i = 0; reject[i] && reject[i] == *haystack; i ++ );

		if( reject[i] ) return ret;
		ret ++;
	}
	return ret;
}

EXPORT size_t strspn(const char *haystack, const char *accept)
{
	size_t	ret = 0;
	 int	i;
	while( *haystack )
	{
		for( i = 0; accept[i] && accept[i] == *haystack; i ++ );

		if( !accept[i] )	return ret;
		ret ++;
	}
	return ret;
}
