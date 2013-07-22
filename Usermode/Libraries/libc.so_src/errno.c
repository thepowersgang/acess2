/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 * 
 * errno.c
 * - errno and strerror
 */
#include "lib.h"
#include <errno.h>
#include <acess/sys.h>
#include <string.h>

EXPORT int *libc_geterrno()
{
	return &_errno;
}

EXPORT char *strerror(int errnum)
{
	switch(errnum)
	{
	case EOK:	return "Success";
	case EISDIR:	return "Is a directory";
	case ENOTDIR:	return "Not a directory";
	case ENOSYS:	return "Invalid instruction/syscall";
	case ENOENT:	return "No such file or directory";
	case EINVAL:	return "Bad arguments";
	case EPERM:	return "Permissions error";
	default:
		_SysDebug("strerror: errnum=%i unk", errnum);
		errno = EINVAL;
		return "unknown error";
	}
}

EXPORT int strerror_r(int errnum, char *buf, size_t bufsiz)
{
	const char *str = strerror(errnum);
	if(!str)
		return -1;
	
	strncpy(buf, str, bufsiz);
	return 0;
}

