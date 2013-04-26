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

// c_iflag
#define	IGNBRK	(1 << 0)
#define	BRKINT	(1 << 1)
#define IGNPAR	(1 << 2)	// Ignore parity failures
#define PARMRK	(1 << 3)	// Mark parity failures with FFh 00h
#define INPCK	(1 << 4)	// Enable input parity checks
#define ISTRIP	(1 << 5)	// strip 8th bit
#define INLCR	(1 << 6)	// Translate input newline into CR
#define IGNCR	(1 << 7)	// Ignore input CR
#define ICRNL	(1 << 8)	// Translate input CR into NL
// (Linux) IUCLC	// Map upper to lower case
#define IXON	(1 <<10)	// Enable input XON/XOFF
#define IXANY	(1 <<11)	// Any character will restart input
#define IXOFF	IXON
// (Linux) IMAXBEL
// (Linux) IUTF8

// c_oflag
#define OPOST	(1 << 0)	// Output processing
// (Linux) OLCUC
#define ONLCR	(1 << 2)	// (XSI) NL->CR
#define OCRNL	(1 << 3)	// CR->NL
#define ONOCR	(1 << 4)	// Don't output CR at column 0
#define ONLRET	(1 << 5)	// Don't output CR
#define OFILL	(1 << 6)	// Send fill chars for a delay, instead of timing
// (Linux) OFDEL
#define NLDLY	(1 << 8)	// Newline delay mask (NL0/NL1)
#define NL0	(0 << 8)
#define NL1	(1 << 8)
#define NCDLY	(3 << 9)	// Carriage return delay mask (CR0-CR3)
#define CR0	(0 << 9)
#define CR1	(1 << 9)
#define CR2	(2 << 9)
#define CR3	(3 << 9)
#define TABDLY	(3 << 11)	// Horizontal tab delay mask
#define TAB0	(0 << 11)
#define TAB1	(1 << 11)
#define TAB2	(2 << 11)
#define TAB3	(3 << 11)
#define BSDLY	(1 << 13)	// Backspace delay
#define BS0	(0 << 13)
#define BS1	(1 << 13)
#define VTDLY	(1 << 14)	// Vertical tab delay
#define VT0	(0 << 14)
#define VT1	(1 << 14)
#define FFDLY	(1 << 15)	// Form feed delay
#define FF0	(0 << 15)
#define VFF1	(1 << 15)

// c_cflag
#define CBAUD	(037 << 0)	// Baud speed
#define CSIZE	(3 << 5)	// Character size mask
#define CS5	(0 << 5)
#define CS6	(1 << 5)
#define CS7	(2 << 5)
#define CS8	(3 << 5)
#define CSTOPB	(1 << 7)	// 1/2 stop bits
#define CREAD	(1 << 8)	// Enable receiver
#define PARENB	(1 << 9)	// Enable parity generation / checking
#define PARODD	(1 << 10)	// Odd parity
#define HUPCL	(1 << 11)	// Hang up on close
#define CLOCAL	(1 << 12)	// Ignore modem control lines
// LOBLK
// CIBAUD
// CMSPAR
// CRTSCTS

// c_lflag
#define ISIG	(1 << 0)	// Generate signals for INTR, QUIT, SUSP and DSUSP
#define ICANON	(1 << 1)	// Input canonical mode
// XCASE
#define ECHO	(1 << 3)	// Echo input characters
#define ECHOE	(1 << 4)	// Enable ERASE and WERASE on echoed input [ICANON]
#define ECHOK	(1 << 5)	// Enable KILL char on echoed input [ICANON]
#define ECHONL	(1 << 6)	// Echo NL even if ECHO is unset [ICANON]
#define ECHOCTL	(1 << 7)	// (non-POSIX) If ECHO set specials are echoed as val+0x40 (BS=^H)
// ECHOPRT
#define ECHOKE	(1 << 9)	// (non-POSIX) KILL implimented by sequential erase [ICANON]
// DEFECHO
// FLUSHO
#define NOFLSH	(1 << 11)	// Disable flush of IO when generating INT, QUIT and SUSP
#define TOSTOP	(1 << 12)	// ?
// PENDIN
#define IEXTEN	(1 << 14)

// - Indexes for c_cc
enum
{
	VDISCARD,
	VDSUSP,
	VEOF,
	VEOL,
	VEOL2,
	VERASE,
	VINTR,
	VKILL,
	VLNEXT,
	VMIN,
	VQUIT,
	// VREPRINT, // (non-POSIX)
	VSTART,
	// VSTATUS, // (non-POSIX)
	VSTOP,
	VSUSP,
	// VSWITCH, // (non-POSIX)
	VTIME,
	VWERASE,
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

