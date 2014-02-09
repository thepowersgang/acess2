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
#include <acess/sys.h>
#include <acess/devices/pty.h>

// === CODE ===
speed_t cfgetospeed(const struct termios *termios_p)
{
	return termios_p->c_oflag & CBAUD;
}

int tcgetattr(int fd, struct termios *termios_p)
{
	if( fd == -1 ) {
		errno = EBADF;
		return -1;
	}
	
	// Double-check `fd` describes a terminal
	if( _SysIOCtl(fd, DRV_IOCTL_TYPE, NULL) != DRV_TYPE_TERMINAL ) {
		errno = EINVAL;
		return -1;
	}
	
	// Fill defaults
	memset(termios_p, 0, sizeof(struct termios));

	// Query kernel for other params
	struct ptymode	mode;
	_SysIOCtl(fd, PTY_IOCTL_GETMODE, &mode);

	if( (mode.OutputMode & PTYOMODE_BUFFMT) != PTYBUFFMT_TEXT ) {
		_SysDebug("Call to tcgetattr when terminal is not in text mode");
		return -1;
	}

	if(mode.InputMode & PTYIMODE_CANON)
		termios_p->c_lflag |= ICANON|ECHOE|ECHOK;
	if(mode.InputMode & PTYIMODE_ECHO)
		termios_p->c_lflag |= ECHO;
	// TODO: The more subtle flags

	return 0;
}

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p)
{
	_SysDebug("tcsetattr(%i, 0%o, %p)", fd, optional_actions, termios_p);

	if( fd == -1 ) {
		errno = EBADF;
		return -1;
	}

	if( !termios_p || (optional_actions & ~(7)) ) {
		errno = EINVAL;
		return -1;
	}
	_SysDebug("*termios_p = {%x,%x,%x,%x}",
		termios_p->c_iflag, termios_p->c_oflag, termios_p->c_cflag, termios_p->c_lflag);
	
	// Double-check `fd` describes a terminal
	if( _SysIOCtl(fd, DRV_IOCTL_TYPE, NULL) != DRV_TYPE_TERMINAL ) {
		errno = EINVAL;
		return -1;
	}

	struct ptymode	mode = {0,0};
	
	if(termios_p->c_lflag & ICANON)
		mode.InputMode |= PTYIMODE_CANON;
	if(termios_p->c_lflag & ECHO)
		mode.InputMode |= PTYIMODE_ECHO;

	_SysIOCtl(fd, PTY_IOCTL_SETMODE, &mode);

	return 0;
}

