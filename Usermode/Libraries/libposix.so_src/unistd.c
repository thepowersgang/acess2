/*
 * Acess2 POSIX Emulation Layer
 * - By John Hodge
 * 
 * unistd.c
 * - POSIX->Acess VFS call translation
 */
#include <unistd.h>
#include <acess/sys.h>
#include <stdarg.h>

// === CODE ===
int open(const char *path, int openmode, ...)
{
	mode_t	create_mode = 0;
	int openflags = 0;
	
	switch( openmode & O_RDWR )
	{
	case 0:	// Special
		break;
	case O_RDONLY:	openflags |= OPENFLAG_READ;	break;
	case O_WRONLY:	openflags |= OPENFLAG_WRITE;	break;
	case O_RDWR:	openflags |= OPENFLAG_READ|OPENFLAG_WRITE;	break;
	}
	
	if( openmode & O_CREAT ) {
		openflags |= OPENFLAG_CREATE;
		va_list	args;
		va_start(args, openmode);
		create_mode = va_arg(args, mode_t);
		va_end(args);
	}
	
	return _SysOpen(path, openflags, create_mode);
}

int creat(const char *path, mode_t mode)
{
	// TODO: Make native call to do this cheaper
	int fd = _SysOpen(path, OPENFLAG_CREATE, mode);
	if( fd == -1 )	return -1;
	_SysClose(fd);
	return 0;
}

int close(int fd)
{
	_SysClose(fd);
	return 0;
}

ssize_t	write(int fd, const void *buf, size_t count)
{
	return _SysWrite(fd, buf, count);
}

ssize_t	read(int fd, void *buf, size_t count)
{
	return _SysRead(fd, buf, count);
}

int fork(void)
{
	return clone(CLONE_VM, 0);
}

int execv(const char *b, char *v[])
{
	return execve(b, v, NULL);
}
