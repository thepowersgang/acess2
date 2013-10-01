/*
 * udibuild - UDI Compilation Utility
 * - By John Hodge (thePowersGang)
 *
 * common.h
 * - Common helpers
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdarg.h>

#ifndef __GNUC__
# define __attribute__(...)
#endif

extern char	*mkstr(const char *fmt, ...) __attribute__((format(printf,1,2)));

#endif

