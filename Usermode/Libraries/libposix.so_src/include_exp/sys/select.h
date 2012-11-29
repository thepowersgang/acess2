/*
 * Acess2 POSIX Emulation Layer
 * - By John Hodge
 * 
 * sys/select.h
 * - select(2)
 */
#ifndef _SYS__SELECT_H_
#define _SYS__SELECT_H_

#include <acess/fd_set.h>

extern int select(int nfds, fd_set *readfds, fd_set *writefds, struct timeval *timeout);
// TODO: pselect?

#endif

