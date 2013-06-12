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

//! Atomic integer type
typedef volatile int	sig_atomic_t;

#define SIG_IGN	((void*)1)
#define SIG_DFL	((void*)0)
#define SIG_ERR	((void*)-1)

#define SIGINT	2	// C99
#define SIGILL	4	// C99
#define	SIGABRT	6	// C99
#define SIGFPE	8	// C99
#define SIGSEGV	11	// C99
#define SIGTERM	15	// C99

extern sighandler_t	signal(int signum, sighandler_t handler);

extern int	raise(int sig);

// POSIX Signals
#define SIGHUP	1
#define SIGQUIT	3
#define SIGKILL	9
#define SIGALRM	14
#define SIGUSR1	16
#define SIGUSR2	17

#define SIGPIPE	1001
#define SIGCHLD	1002

typedef int	sigset_t;

#endif

