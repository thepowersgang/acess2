/**
 * \file strmem.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>

// === EXPORTS ===
EXPORT(udi_snprintf);

// === CODE ===
udi_size_t udi_snprintf(char *s, udi_size_t max_bytes, const char *format, ...)
{
	udi_size_t	ret;
	va_list	args;
	va_start(args, format);
	
	ret = vsnprintf(s, max_bytes, format, args);
	
	va_end(args);
	return ret;
}
