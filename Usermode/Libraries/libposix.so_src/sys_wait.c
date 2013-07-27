/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * sys_wait.c
 * - Waiting
 */
#include <sys/wait.h>
#include <acess/sys.h>
#include <errno.h>

// === CODE ===
pid_t wait(int *status)
{
	return _SysWaitTID(0, status);
}

pid_t waitpid(pid_t pid, int *status, int options)
{
	if( options & WNOHANG ) {
		_SysDebug("TODO: support waitpid(...,WNOHANG)");
		errno = EINVAL;
		return -1;
	}
	return _SysWaitTID(pid, status);
}

