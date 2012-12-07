/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * dirent.c
 * - Directory Functions
 */
#include <dirent.h>
#include <stddef.h>	// NULL
#include <acess/sys.h>
#include <errno.h>

struct DIR_s
{
	 int	fd;
	 int	pos;
	struct dirent	tmpent;
};

// === CODE ===
DIR *fdopendir(int fd)
{
	t_sysFInfo	info;
	if( _SysFInfo(fd, &info, 0) != 0 )
		return NULL;

	

	return NULL;
}

DIR *opendir(const char *name)
{
	int fd = _SysOpen(name, OPENFLAG_READ);
	if( fd == -1 )
		return NULL;
	
	DIR *ret = fdopendir(fd);
	if( !ret ) {
		_SysClose(fd);
	}
	return ret;
}

int closedir(DIR *dp)
{
	if( !dp ) {
		errno = EINVAL;
		return -1;
	}
	return 0;
}

struct dirent *readdir(DIR *dp)
{
	if( !dp ) {
		errno = EINVAL;
		return NULL;
	}

	errno = EOK;
	int rv = _SysReadDir(dp->fd, dp->tmpent.d_name);
	if( rv != 1 ) {
		// TODO: Fix kernel-land API
		return NULL;
	}
	dp->tmpent.d_ino = 0;

	return &dp->tmpent;
}

extern int	readdir_r(DIR *, struct dirent *, struct dirent **);
extern void	rewinddir(DIR *);
extern void	seekdir(DIR *, long int);
extern long int	telldir(DIR *);
