/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * sys_ioctl.c
 * - IOCtl hacks
 */
#include <sys/ioctl.h>
#include <acess/sys.h>
#include <errno.h>

// === CODE ===
int ioctl(int fd, int request, ...)
{
	if( fd < 0 ) {
		errno = EBADF;
		return -1;
	}
	
	if( request < 0 ) {
		errno = EINVAL;
		return -1;
	}

	// #1. Get device type (IOCtl 0)
	int devtype = _SysIOCtl(fd, 0, NULL);
	
	switch(devtype)
	{
	// 0: Normal file (no ioctls we care about)
	case 0:
	// 1: Has the ident set of ioctls, nothing else
	case 1:
		return -1;
	// TODO: Terminals
	
	// NFI
	default:
		return -1;
	}

	return 0;
}


