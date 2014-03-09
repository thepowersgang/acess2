/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * stdio_files.c
 * - non-stream file manipulation
 */
#include <stdio.h>
#include <acess/sys.h>	// _SysDebug

// === CODE ===
int remove(const char *filename)
{
	_SysDebug("TODO: libc remove('%s')", filename);
	return 1;
}

int rename(const char *old, const char *new)
{
	_SysDebug("TODO: libc rename('%s','%s')", old, new);
	return 1;
}

