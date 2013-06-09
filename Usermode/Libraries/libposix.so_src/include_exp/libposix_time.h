/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * time.h (libposix version)
 * - Time and Clock definitions
 *
 * NOTE: Included by libc's time.h
 */
#ifndef _LIBPOSIX__TIME_H_
#define _LIBPOSIX__TIME_H_

struct timespec
{
	time_t	tv_sec;
	long	tv_nsec;
};

enum clockid_e
{
	CLOCK_REALTIME,
	CLOCK_MONOTONIC
};

typedef enum clockid_e	clockid_t;

extern int clock_getres(clockid_t clk_id, struct timespec *res);
extern int clock_gettime(clockid_t clk_id, struct timespec *tp);
extern int clock_settime(clockid_t clk_id, const struct timespec *tp);

#endif

