/*
 * Acess2 User Core
 * - By John Hodge (thePowersGang)
 * 
 * acess/devices/pty.h
 * - PTY Device type IOCtls
 */
#ifndef _ACESS_DEVICES_PTY_H_
#define _ACESS_DEVICES_PTY_H_


#include "../devices.h"

#define PTYSFLAG_ILOCAL	0x001	//!< Input modes are handled in the server process
#define PTYSFLAG_SMODE	0x002	//!< Inform server of mode changes by the user

#define PTYIMODE_CANON	0x001	//!< Line-buffered input
#define PTYIMODE_ECHO	0x002	//!< Echo input characters
#define PTYIMODE_RAW	0x004	//!< Disable all input processing
#define PTYIMODE_NLCR	0x008	//!< Translate '\n' to '\r' (for old apps)

#define PTYOMODE_BUFFMT	0x003
#define PTYBUFFMT_TEXT	 0x000
#define PTYBUFFMT_FB	 0x001
#define PTYBUFFMT_2DCMD	 0x002
#define PTYBUFFMT_3DCMD	 0x003

/*
 * Note: When setting dimensions from a client, it is up to the server what fields are used.
 * This is usually dependent on the current output mode.
 */
struct ptydims
{
	short	W;
	short	H;
	short	PW;
	short	PH;
} PACKED;
struct ptymode
{
	unsigned int	OutputMode;
	unsigned int	InputMode;
} PACKED;

enum
{
	PTY_IOCTL_GETMODE = 4,
	PTY_IOCTL_SETMODE,
	PTY_IOCTL_GETDIMS,
	PTY_IOCTL_SETDIMS,
	PTY_IOCTL_GETID,
	PTY_IOCTL_SETID,
	PTY_IOCTL_SETPGRP
};

#endif

