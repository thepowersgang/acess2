/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * termios.c
 * - Terminal Control
 */
#include <termios.h>
#include <errno.h>
#include <string.h>

// === CODE ===
int tcgetattr(int fd, struct termios *termios_p)
{
	if( fd == -1 ) {
		errno = EBADF;
		return -1;
	}
	// Double-check `fd` describes a terminal
	
	// Fill defaults
	memset(termios_p, 0, sizeof(struct termios));

	// Query kernel for other params	

	return 0;
}

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p)
{
	if( fd == -1 ) {
		errno = EBADF;
		return -1;
	}

	if( !termios_p || (optional_actions & ~(7)) ) {
		errno = EINVAL;
		return -1;
	}
	
	return 0;
}

