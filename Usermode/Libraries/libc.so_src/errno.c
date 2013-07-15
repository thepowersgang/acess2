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

EXPORT int *libc_geterrno()
{
	return &_errno;
}

EXPORT const char *strerror(int errnum)
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
		return "unknown error";
	}
}

