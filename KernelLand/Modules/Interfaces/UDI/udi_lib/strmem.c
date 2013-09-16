/**
 * \file strmem.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>

// === EXPORTS ===
EXPORTAS(strlen, udi_strlen);
EXPORTAS(strcat, udi_strcat);
EXPORTAS(strncat, udi_strncat);
EXPORTAS(strcmp, udi_strcmp);
EXPORTAS(strncmp, udi_strncmp);
EXPORTAS(memcmp, udi_memcmp);
EXPORTAS(strcpy, udi_strcpy);
EXPORTAS(strncpy, udi_strncpy);
EXPORTAS(memcpy, udi_memcpy);
EXPORTAS(memmove, udi_memmove);
EXPORT(udi_strncpy_rtrim);
EXPORTAS(strchr, udi_strchr);
EXPORTAS(strrchr, udi_strrchr);
EXPORT(udi_memchr);
EXPORTAS(memset, udi_memset);
EXPORT(udi_strtou32);
EXPORT(udi_snprintf);
EXPORT(udi_vsnprintf);

// === CODE ===
char *udi_strncpy_rtrim(char *s1, const char *s2, udi_size_t n)
{
	char *dst = s1;
	while( n-- )
	{
		*dst++ = *s2++;
	}
	while( dst > s1 && isspace(*--dst) )
		;
	dst ++;
	*dst = '\0';
	return s1;
}

void *udi_memchr(const void *s, udi_ubit8_t c, udi_size_t n)
{
	const udi_ubit8_t *p = s;
	while(n--)
	{
		if( *p == c )
			return (void*)p;
		p ++;
	}
	return NULL;
}

udi_ubit32_t udi_strtou32(const char *s, char **endptr, int base)
{
	return strtoul(s, endptr, base);
}

udi_size_t udi_snprintf(char *s, udi_size_t max_bytes, const char *format, ...)
{
	udi_size_t	ret;
	va_list	args;
	va_start(args, format);
	
	ret = udi_vsnprintf(s, max_bytes, format, args);
	
	va_end(args);
	return ret;
}
udi_size_t udi_vsnprintf(char *s, udi_size_t max_bytes, const char *format, va_list ap)
{
	// TODO: This should support some stuff Acess doesn't
	return vsnprintf(s, max_bytes, format, ap);
}
