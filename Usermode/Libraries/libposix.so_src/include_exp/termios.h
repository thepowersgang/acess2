/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * termios.h
 * - Terminal Control
 */
#ifndef _LIBPOSIX__TERMIOS_H_
#define _LIBPOSIX__TERMIOS_H_

typedef unsigned char	cc_t;
typedef unsigned long	speed_t;
typedef unsigned short	tcflag_t;

enum {
	VEOF,
	VEOL,
	VERASE,
	VINTR,
	VKILL,
	VMIN,
	VQUIT,
	VSTART,
	VSTOP,
	VSUSP,
	VTIME,
	NCCS
};

struct termios
{
	tcflag_t	c_iflag;
	tcflag_t	c_oflag;
	tcflag_t	c_cflag;
	tcflag_t	c_lflag;
	cc_t	c_cc[NCCS];
};

#endif

