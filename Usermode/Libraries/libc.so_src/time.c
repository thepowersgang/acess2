/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * time.c
 * - POSIX/stdc time functions
 */
#include <time.h>
#include <acess/sys.h>
#include <string.h>
#include "timeconv.h"
#include <errno.h>

#define UNIX_TO_2K	((30*365*3600*24) + (7*3600*24))	//Normal years + leap years

clock_t clock(void)
{
	return _SysTimestamp();
}

time_t mktime(struct tm *timeptr)
{
	time_t ret = seconds_since_y2k(
		timeptr->tm_year - 2000,
		timeptr->tm_mon,
		timeptr->tm_mday,
		timeptr->tm_hour,
		timeptr->tm_min,
		timeptr->tm_sec
		);
	if( ret == 0 && errno ) {
		// Bad date
	}
	ret += UNIX_TO_2K;
	
	return ret;
}

time_t time(time_t *t)
{
	time_t ret = _SysTimestamp() / 1000;
	if(t)
		*t = ret;
	return ret;
}

static struct tm	static_tm;

struct tm *localtime(const time_t *timer)
{
	return localtime_r(timer, &static_tm);

}
struct tm *localtime_r(const time_t *timer, struct tm *ret)
{
	// Hours, Mins, Seconds
	int64_t	days = get_days_since_y2k(*timer, &ret->tm_hour, &ret->tm_min, &ret->tm_sec);
	
	// Week day
	ret->tm_wday = (days + 6) % 7;	// Sun = 0, 1 Jan 2000 was Sat (6)
	
	// Year and Day of Year
	bool	is_ly;
	ret->tm_year = 2000 + get_years_since_y2k(days, &is_ly, &ret->tm_yday);

	// Month and Day of Month
	get_month_day(ret->tm_yday, is_ly, &ret->tm_mon, &ret->tm_mday);
	
	ret->tm_isdst = 0;	// Fuck DST
	
	return ret;
}

static inline size_t MIN(size_t a, size_t b) { return a < b ? a : b; }
static size_t _puts(char * restrict s, size_t maxsize, size_t ofs, const char *str, size_t len)
{
	if( s && ofs < maxsize ) {
		size_t	addlen = MIN(len, maxsize-1-ofs);
		memcpy(s+ofs, str, addlen);
	}
	return len;
}

size_t strftime(char*restrict s, size_t maxsize, const char*restrict format, const struct tm*restrict timeptr)
{
	size_t	ofs = 0;

	while( *format )
	{
		const char *restrict start = format;
		while( *format && *format != '%' )
			format ++;
		if( format != start )
			ofs += _puts(s, maxsize, ofs, start, format-start);
		if( *format == 0 )
			break;
		format ++;
		switch(*format++)
		{
		case 0:	format--;	break;
		case '%':	ofs += _puts(s, maxsize, ofs, format-1, 1);	break;
		case 'd':	// The day of the month as a decimal number (range 01 to 31).
			{
			char tmp[2] = {'0','0'};
			tmp[0] += (timeptr->tm_mday / 10) % 10;
			tmp[1] += timeptr->tm_mday % 10;
			ofs += _puts(s, maxsize, ofs, tmp, 2);
			}
			break;
		default:
			_SysDebug("TODO: strftime('...%%%c...')", format[-1]);
			break;
		}
	}
	
	return ofs;
}
