/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * sys/time.h
 * - Timing calls
 */
#ifndef _LIBPOSIX__SYS__TIME_H_
#define _LIBPOSIX__SYS__TIME_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long	suseconds_t;

struct timeval
{
	time_t	tv_sec;
	suseconds_t	tv_usec;
};

// struct timezone . tz_dsttime
enum
{
	DST_NONE,	// No DST
	DST_USA,	// USA style DST
	DST_AUST,	// Australian style DST
	DST_WET,	// Western European DST
	DST_MET,	// Middle European DST
	DST_EET,	// Eastern European DST
	DST_CAN,	// Canada
	DST_GB, 	// Great Britain and Eire
	DST_RUM,	// Rumania
	DST_TUR,	// Turkey
	DST_AUSTALT,	// Australia with 1986 shift
};

struct timezone
{
	int tz_minuteswest;
	int tz_dsttime;
};

struct itimerval
{
	struct timeval	it_interval;
	struct timeval	it_value;
};

// TODO: This should also define fd_set and select()

extern int	getitimer(int which, struct itimerval *current_value);
extern int	setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value);
extern int	gettimeofday(struct timeval *tv, struct timezone *tz);
// extern int	settimeofday(const struct timeval *tv, const struct timezone *tz); //ifdef _BSD_SOURCE
// select
extern int	utimes(const char *, const struct timeval [2]);

#ifdef __cplusplus
}
#endif

#endif

