/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * sys_ioctl.c
 * - IOCtl hacks
 */
#include <sys/ioctl.h>
#include <acess/sys.h>
#include <acess/devices.h>	// DRV_TYPE_*
#include <acess/devices/pty.h>
#include <errno.h>
#include <termios.h>	// TIOC*
#include <stdarg.h>

// === CODE ===
int ioctl(int fd, int request, ...)
{
	va_list	args;
	va_start(args, request);
	
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
		_SysDebug("ioctl(%i, %i, ...) (File)", fd, request);
		return -1;
	// 1: Has the ident set of ioctls, nothing else
	case 1:
		_SysDebug("ioctl(%i, %i, ...) (Misc Dev)", fd, request);
		return -1;
	
	// TODO: Terminals
	case DRV_TYPE_TERMINAL:
		switch(request)
		{
		case TIOCGWINSZ: {
			struct winsize *ws = va_arg(args, struct winsize*);
			_SysDebug("ioctl(%i, TIOCGWINSZ, %p", fd, ws);
			struct ptydims	dims;
			_SysIOCtl(fd, PTY_IOCTL_GETDIMS, &dims);
			ws->ws_col = dims.W;
			ws->ws_row = dims.H;
			ws->ws_xpixel = dims.PW;
			ws->ws_ypixel = dims.PH;
			return 0; }
		case TIOCSWINSZ: {
			const struct winsize *ws = va_arg(args, const struct winsize*);
			_SysDebug("ioctl(%i, TIOCSWINSZ, %p", fd, ws);
			struct ptydims	dims;
			dims.W = ws->ws_col;
			dims.H = ws->ws_row;
			dims.PW = ws->ws_xpixel;
			dims.PH = ws->ws_ypixel;
			_SysIOCtl(fd, PTY_IOCTL_SETDIMS, &dims);
			return 0; }
		default:
			_SysDebug("ioctl(%i, TIOC? %i, ...)", fd, request);
			break;
		}
		return -1;
	
	// NFI
	default:
		_SysDebug("ioctl(%i, %i, ...) (DevType %i)", fd, request, devtype);
		return -1;
	}

	va_end(args);
	return 0;
}


