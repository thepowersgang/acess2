/*
 * Acess2 POSIX Emulation Lib
 * - By John Hodge (thePowersGang)
 *
 * sys/stat.h
 * - stat(2)
 */
#include <sys/stat.h>
#include <acess/sys.h>

// === CODE ===
int stat(const char *path, struct stat *buf)
{
	int fd = _SysOpen(path, 0);
	if( !fd ) {
		// errno is set by _SysOpen
		return -1;
	}
	
	int rv = fstat(fd, buf);
	_SysClose(fd);
	return rv;
}

int lstat(const char *path, struct stat *buf)
{
	int fd = _SysOpen(path, OPENFLAG_NOLINK);
	if( !fd ) {
		// errno is set by _SysOpen
		return -1;
	}
	
	int rv = fstat(fd, buf);
	_SysClose(fd);
	return rv;
}

int fstat(int fd, struct stat *buf)
{
	t_sysFInfo	info;
	
	int rv = _SysFInfo(fd, &info, 0);
	if( !rv ) {
		return -1;
	}

	// TODO: Populate buf
	buf->st_mode = 0;
//	memset(buf, 0, sizeof(*buf));
	
	return 0;
}

