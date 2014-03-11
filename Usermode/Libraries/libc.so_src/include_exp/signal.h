/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * signal.h
 * - POSIX Signal Emulation/Interface
 */
#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "signal_list.h"

typedef void (*sighandler_t)(int);

//! Atomic integer type
typedef volatile int	sig_atomic_t;

#define SIG_IGN	((sighandler_t)1)
#define SIG_DFL	((sighandler_t)0)
#define SIG_ERR	((sighandler_t)-1)

extern sighandler_t	signal(int signum, sighandler_t handler);

extern int	raise(int sig);

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

#if __cplusplus
}
#endif

#endif

