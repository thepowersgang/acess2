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
#include <sys/select.h>
#include <stdio.h>

// === CODE ===
int unlink(const char *pathname)
{
	return _SysUnlink(pathname);
}

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

int seek(int fd, int whence, off_t dest)
{
	return _SysSeek(fd, whence, dest);
}

off_t tell(int fd)
{
	return _SysTell(fd);
}

int fork(void)
{
	return _SysClone(CLONE_VM, 0);
}

int execv(const char *b, char *v[])
{
	return _SysExecVE(b, v, NULL);
}

int dup(int oldfd)
{
	// NOTE: Acess's CopyFD doesn't cause offset sharing
	// TODO: Check that -1 does cause a new allocation
	return _SysCopyFD(oldfd, -1);
}

int dup2(int oldfd, int newfd)
{
	// NOTE: Acess's CopyFD doesn't cause offset sharing
	return _SysCopyFD(oldfd, newfd);
}


/*
 * Set session ID to PID
 */
pid_t setsid(void)
{
	// TODO: actual syscall for this
	return _SysGetPID();
}

pid_t getpid(void)
{
	return _SysGetPID();
}

uid_t getuid(void)
{
	return _SysGetUID();
}

uid_t geteuid(void)
{
	// TODO: Impliment EUIDs in-kernel?
	return _SysGetUID();
}

int kill(pid_t pid, int signal)
{
	// TODO: Need special handling?
	return _SysKill(pid, signal);
}

int select(int nfd, fd_set *rfd, fd_set *wfd, fd_set *efd, struct timeval *timeout)
{
	
	if( timeout )
	{
		long long int ltimeout = 0;
		ltimeout = timeout->tv_sec*1000 + timeout->tv_usec / 1000;
		int ret = _SysSelect(nfd, rfd, wfd, efd, &ltimeout, 0);
		return ret;
	}
	else
	{
		return _SysSelect(nfd, rfd, wfd, efd, NULL, 0);
	}
}

int pipe(int pipefd[2])
{
	pipefd[0] = _SysOpen("/Devices/fifo/anon", OPENFLAG_READ|OPENFLAG_WRITE);
	pipefd[1] = _SysCopyFD(pipefd[0], -1);
	_SysFDFlags(pipefd[1], OPENFLAG_READ|OPENFLAG_WRITE, OPENFLAG_WRITE);
	return 0;
}

int chdir(const char *dir)
{
	return _SysChdir(dir);
}

int mkdir(const char *pathname, mode_t mode)
{
	_SysDebug("TODO: POSIX mkdir(%i, 0%o)", pathname, mode);
	return -1;
}

char *getpass(const char *prompt)
{
	static char passbuf[PASS_MAX+1];
	fprintf(stderr, "%s", prompt);
	fgets(passbuf, PASS_MAX+1, stdin);
	fprintf(stderr, "\n");
	return passbuf;
}

