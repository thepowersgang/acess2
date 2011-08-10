/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * time.h
 * - POSIX time functions
 */
#ifndef _TIME_H_
#define _TIME_H_

struct tm
{
	 int	tm_sec;
	 int	tm_min;
	 int	tm_hour;
	 int	tm_mday;
	 int	tm_mon;
	 int	tm_year;	// 1900 based
	 int	tm_wday;
	 int	tm_yday;
	 int	tm_isdst;
};

#endif

