/*
 * Acess2 POSIX Emulation Layer
 * - By John Hodge
 * 
 * pty.c
 * - PTY Management
 *
 * BSD Conforming (Non-POSIX)
 */
#include <pty.h>
#include <errno.h>
#include <unistd.h>
#include <acess/sys.h>
#include <acess/devices.h>
#include <acess/devices/pty.h>

// === CODE ===
int openpty(int *amaster, int *aslave, char *name, const struct termios *termp, const struct winsize *winp)
{
	errno = ENOTIMPL;
	return -1;
}

pid_t forkpty(int *amaster, char *name, const struct termios *termp, const struct winsize *winp)
{
	int child;
	
	int ret = openpty(amaster, &child, name, termp, winp);
	if(ret)	return -1;
	
	pid_t rv = fork();
	if(rv)	return rv;

	login_tty(child);

	// In child
	dup2(child, 0);
	dup2(child, 1);
	dup2(child, 2);
	close(child);

	return 0;
}

// - utmp.h?
int login_tty(int fd)
{
	// nop!
	return 0;
}
