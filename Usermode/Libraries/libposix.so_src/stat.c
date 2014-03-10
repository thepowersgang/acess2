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
	if( rv == -1 ) {
		return -1;
	}

	_SysDebug("fstat(fd=%i,buf=%p)", fd, buf);

	// TODO: Populate buf
	buf->st_dev = info.mount;
	buf->st_ino = info.inode;
	if( info.flags & FILEFLAG_SYMLINK )
		buf->st_mode = S_IFLNK;
	else if( info.flags & FILEFLAG_DIRECTORY )
		buf->st_mode = S_IFDIR;
	else
		buf->st_mode = S_IFREG;
	// TODO: User modes (assume 660)
	buf->st_mode |= 0660;
	buf->st_nlink = 1;
	buf->st_uid = info.uid;
	buf->st_gid = info.gid;
	buf->st_rdev = NULL;
	buf->st_size = info.size;
	buf->st_blksize = 512;
	buf->st_blocks = (info.size+511)/512;
	buf->st_atime = info.atime;
	buf->st_mtime = info.mtime;
	buf->st_ctime = info.ctime;
	
	return 0;
}

