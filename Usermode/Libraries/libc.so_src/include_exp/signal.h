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

#define SIG_IGN	((void*)1)
#define SIG_DFL	((void*)0)
#define SIG_ERR	((void*)-1)

#define SIGHUP	1
#define SIGINT	2
#define SIGQUIT	3
#define SIGILL	4
#define	SIGABRT	6
#define SIGFPE	8
#define SIGKILL	9
#define SIGSEGV	11
//#define SIGPIPE	13
#define SIGALRM	14
#define SIGTERM	15
#define SIGUSR1	16
#define SIGUSR2	17

#define SIGPIPE	1001
#define SIGCHLD	1002

extern sighandler_t	signal(int signum, sighandler_t handler);

extern int	raise(int sig);

#endif

