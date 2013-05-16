/*
 * Acess2 POSIX Emulation Library
 * - By John Hodge (thePowersGang)
 *
 * fcntl.c
 * - File descriptor control
 */
#include <sys/types.h>	// off_t
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <acess/sys.h>
#include <errno.h>

// === CODE ===
int fcntl(int fd, int cmd, ...)
{
	 int	ret;
	va_list	args;
	va_start(args, cmd);
	
	switch(cmd)
	{
	case F_GETFL: {
		 int	a_flags = _SysFDFlags(fd, 0, 0);
		if( a_flags == -1 )
			return -1;
		ret = 0;
		if(a_flags & OPENFLAG_READ)	ret |= O_RDONLY;
		if(a_flags & OPENFLAG_WRITE)	ret |= O_WRONLY;
		if(a_flags & OPENFLAG_NONBLOCK)	ret |= O_NONBLOCK;
		if(a_flags & OPENFLAG_APPEND) 	ret |= O_APPEND;
		// TODO: Extra flags for F_GETFL
		break; }
	case F_SETFL: {
		long	p_flags = va_arg(args, long);
		 int	a_flags = 0;
		const int	mask = OPENFLAG_NONBLOCK|OPENFLAG_APPEND;
		
		if(p_flags & O_NONBLOCK)
			a_flags |= OPENFLAG_NONBLOCK;
		if(p_flags & O_APPEND)
			a_flags |= OPENFLAG_APPEND;
		// TODO: Extra flags for F_SETFL

		ret = _SysFDFlags(fd, mask, a_flags);
		_SysDebug("fcntl(%i, F_SETFL, %li) = %i", fd, p_flags, ret);
		if(ret != -1)
			ret = 0;

		break; }
	default:
		_SysDebug("fcntl(%i) unknown or unimplimented", cmd);
		errno = EINVAL;
		ret = -1;
		break;
	}
	va_end(args);
	return ret;
}
