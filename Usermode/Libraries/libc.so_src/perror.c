/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * perror.c
 * - perror() and friends
 */
#include <errno.h>
#include <stdio.h>
#include <acess/sys.h>

void perror(const char *s)
{
	 int	errnum = errno;
	_SysDebug("perror(): %s: Error (%i) %s", s, errnum, strerror(errnum));
	fprintf(stderr, "%s: Error (%i) %s\n", s, errnum, strerror(errnum));
}
