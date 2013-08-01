/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * time.h
 * - C99 Time Functions
 */
#ifndef _TIME_H_
#define _TIME_H_

#include <sys/types.h>	// time_t
#include <stddef.h>	// size_t

struct tm
{
	 int	tm_sec;	// 0-60
	 int	tm_min;	// 0-59
	 int	tm_hour;
	 int	tm_mday;
	 int	tm_mon;
	 int	tm_year;	// 1900 based
	 int	tm_wday;
	 int	tm_yday;
	 int	tm_isdst;
};

#define CLOCKS_PER_SEC	1000

typedef signed long long	clock_t;

//! Processor time used since invocation
extern clock_t	clock(void);

//! Difference between two times in seconds
extern double difftime(time_t time1, time_t time0);

//! Convert a local time into the format returned by \a time
//! \note Also updates tm_wday and tm_yday to the correct values
extern time_t	mktime(struct tm *timeptr);

//! Get the current calendar time
//! \note -1 is returned if the calendar time is not avaliable
extern time_t	time(time_t *t);

//
// Time conversions
//
//! Convert the time structure into a string of form 'Sun Sep 16 01:03:52 1973\n\0'
extern char *asctime(const struct tm *timeptr);

//! asctime(localtime(timer))
extern char *ctime(const time_t *timer);

//! Convert \a timter into UTC
extern struct tm *gmtime(const time_t *timer);

extern struct tm *localtime(const time_t *timer);

extern size_t strftime(char*restrict s, size_t maxsize, const char*restrict format, const struct tm*restrict timeptr);

#include <libposix_time.h>

#endif

