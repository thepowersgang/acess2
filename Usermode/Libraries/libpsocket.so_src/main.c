/*
 * Acess2 POSIX Sockets Library
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Entrypoint and misc code
 */
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "common.h"

int SoMain(void)
{
	return 0;
}

char *mkstr(const char *format, ...)
{
	va_list	args;
	 int	len;
	char	*ret;
	
	va_start(args, format);
	len = vsnprintf(NULL, 0, format, args);
	va_end(args);
	ret = malloc(len + 1);
	va_start(args, format);
	vsnprintf(ret, len+1, format, args);
	va_end(args);
	return ret;
}

