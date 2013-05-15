/*
 * Acess2 POSIX Emulation Library
 * - By John Hodge (thePowersGang)
 *
 * fcntl.h
 * - File descriptor control?
 */

#ifndef _FCNTL_H_
#define _FCNTL_H_

struct flock
{
	short	l_type;
	short	l_whece;
	off_t	l_start;
	off_t	l_len;
	pid_t	l_pid;
};

enum e_fcntl_cmds
{
	F_DUPFD,	// (long)
	//F_DUPFD_CLOEXEC,	// (long) - Non POSIX
	F_GETFD,	// (void)
	F_SETFD,	// (long)
	F_GETFL,	// (void)
	F_SETFL,	// (long)
	
	F_SETLK,	// (struct flock *)
	F_SETLKW,	// (struct flock *)
	F_GETLK,	// (struct flock *)
};

extern int fcntl(int fd, int cmd, ...);

#endif

