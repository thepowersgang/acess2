/**
 * \file strmem.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>
#include <udi_strmem.h>

// === CODE ===
udi_size_t udi_snprintf(char *s, udi_size_t max_bytes, const char *format, ...)
{
	s[0] = '\0';
	return 0;
}

// === EXPORTS ===
EXPORT(udi_snprintf);
