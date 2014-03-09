/*
 * Acess2 POSIX Emulation Layer
 * - By John Hodge
 * 
 * utime.h
 * - 
 */
#include <utime.h>
#include <acess/sys.h>

// === CODE ===
int utime(const char *filename, const struct utimbuf *times)
{
	_SysDebug("TODO: libposix utime('%s', {%lli,%lli})",
		filename, times->actime, times->modtime
		);
	return 0;
}

