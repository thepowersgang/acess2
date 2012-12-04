/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * signal.h
 * - POSIX Signal Emulation/Interface
 */
#ifndef _SIGNAL_H_
#define _SIGNAL_H_

typedef void (*sighandler_t)(int);

#define SIG_DFL	((void*)0)
#define SIG_ERR	((void*)-1)

#define	SIGABRT	6

#define SIGPIPE	1001
#define SIGCHLD	1002

extern int	raise(int sig);

#endif

