/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * sys/time.h
 * - Timing calls
 */
#ifndef _LIBPOSIX__SYS__TIME_H_
#define _LIBPOSIX__SYS__TIME_H_

typedef unsigned long	suseconds_t;

struct timeval
{
	time_t	tv_sec;
	suseconds_t	tv_usec;
};

struct itimerval
{
	struct timeval	it_interval;
	struct timeval	it_value;
};

// TODO: This should also define fd_set and select()

extern int	getitimer(int, struct itimerval *);
extern int	setitimer(int, const struct itimerval *, struct itimerval *);
extern int	gettimeofday(struct timeval *, void *);
// select
extern int	utimes(const char *, const struct timeval [2]);

#endif

