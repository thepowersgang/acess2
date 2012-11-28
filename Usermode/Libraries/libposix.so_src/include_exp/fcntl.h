/*
 * Acess2 C Library (UNIX Emulation)
 * - By John Hodge (thePowersGang)
 *
 * fcntl.h
 * - ??
 */

#ifndef _FCNTL_H_
#define _FCNTL_H_

#include <sys/sys.h>

// Hacks to handle different behaviors in Acess

// Open doesn't take a chmod
#define open(_1,_2,...)	open(_1, _2)

// Close returns void
#define close(_1)	(close(_1),0)

// Acess doesn't implement lseek
#define lseek(_1,_2,_3)	(seek(_1,_2,_3),tell(_1))

enum e_fcntl_cmds
{
	F_SETFL
};

int fcntl(int fd, int cmd, ...) { return -1; }

#endif

