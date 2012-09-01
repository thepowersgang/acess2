/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * perror.c
 * - perror() and friends
 */
#include <errno.h>
#include <stdio.h>

void perror(const char *s)
{
	fprintf(stderr, "%s: Error (%i)\n", s, errno);
}
