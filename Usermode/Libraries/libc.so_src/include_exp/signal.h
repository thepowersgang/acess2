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

#define SIGSTOP	30	// Stop process
#define SIGTSTP	31	// ? ^Z
#define SIGTTIN	32	// Background process read TTY
#define SIGTTOU	33	// Background process write TTY
#define SIGPIPE	34
#define SIGCHLD	35
#define SIGWINCH	36

#include <sys/types.h>	// libposix

typedef long long unsigned int	sigset_t;
extern int	sigemptyset(sigset_t *set);
extern int	sigfillset(sigset_t *set);

typedef struct siginfo_s	siginfo_t;

struct siginfo_s
{
	int	si_signo;
	int	si_errno;
	int	si_code;
	int	si_trapno;
	pid_t	si_pid;
	uid_t	si_uid;
	int	si_status;
	// TODO: There's others
};

struct sigaction
{
	sighandler_t	sa_handler;
	//void	(*sa_sigaction)(int, siginfo_t *, void *);
	sigset_t	sa_mask;
	int	sa_flags;
};

#define SA_NOCLDSTOP	0x001

extern int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);

#endif

