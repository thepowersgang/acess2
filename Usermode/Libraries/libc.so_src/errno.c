/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 * 
 * errno.c
 * - errno and strerror
 */
#include "lib.h"
#include <stdio.h>
#include <errno.h>
#include <acess/sys.h>

EXPORT int *libc_geterrno()
{
	return &_errno;
}

EXPORT const char *strerror(int errnum)
{
	switch(errnum)
	{
	case ENOSYS:	return "Invalid instruction/syscall";
	case ENOENT:	return "No such file or directory";
	case EINVAL:	return "Bad arguments";
	case EPERM:	return "Permissions error";
	default:
		_SysDebug("strerror: errnum=%i unk", errnum);
		return "unknown error";
	}
}

// stdio.h
EXPORT void perror(const char *s)
{
	int err = errno;
	if( s && s[0] ) {
		fprintf(stderr, "%s: (%i) %s\n", s, err, strerror(err));
	}
	else {
		fprintf(stderr, "(%i) %s\n", err, strerror(err));
	}
	_SysDebug("perror('%s'): %s (%i)", s, strerror(err), err);
}

