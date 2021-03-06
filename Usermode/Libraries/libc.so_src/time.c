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
#include <stdio.h>	// sprintf

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

//! Convert the time structure into a string of form 'Sun Sep 16 01:03:52 1973\n\0'
char *asctime(const struct tm *timeptr)
{
	static char staticbuf[sizeof("Sun Sep 16 01:03:52 1973\n")];
	return asctime_r(timeptr, staticbuf);
}
char *asctime_r(const struct tm *timeptr, char *buf)
{
	const char *WDAYS[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
	const char *MONS[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	sprintf(buf,
		"%s %s %i %02i:%02i:%02i %4i\n",
		WDAYS[timeptr->tm_wday],
		MONS[timeptr->tm_mon],
		timeptr->tm_mday,
		timeptr->tm_hour,
		timeptr->tm_min,
		timeptr->tm_sec,
		timeptr->tm_year
		);
	return buf;
}

//! asctime(localtime(timer))
char *ctime(const time_t *timer)
{
	struct tm	time_buf;
	localtime_r(timer, &time_buf);
	return asctime(&time_buf);
}
extern char *ctime_r(const time_t *timer, char *buf)
{
	struct tm	time_buf;
	localtime_r(timer, &time_buf);
	return asctime_r(&time_buf, buf);
}

//! Convert \a timter into UTC
struct tm *gmtime(const time_t *timer)
{
	// Ignore UTC
	return localtime(timer);
}
struct tm *gmtime_r(const time_t *timer, struct tm *result)
{
	return localtime_r(timer, result);
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
	ret->tm_mon --;
	
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
		
		// If EOS is hit on a '%', break early
		if( *format == 0 )
			break;
		switch(*format++)
		{
		// Literal '%', 
		case '%':
			ofs += _puts(s, maxsize, ofs, format-1, 1);
			break;
		// The day of the month as a decimal number (range 01 to 31).
		case 'd':
			{
			char tmp[2] = {'0','0'};
			tmp[0] += (timeptr->tm_mday / 10) % 10;
			tmp[1] += timeptr->tm_mday % 10;
			ofs += _puts(s, maxsize, ofs, tmp, 2);
			}
			break;
		// Two-digit 24 hour
		case 'H':
			{
			char tmp[2] = {'0','0'};
			tmp[0] += (timeptr->tm_hour / 10) % 10;
			tmp[1] +=  timeptr->tm_hour % 10;
			ofs += _puts(s, maxsize, ofs, tmp, 2);
			}
			break;
		// Two-digit minutes
		case 'M':
			{
			char tmp[2] = {'0','0'};
			tmp[0] += (timeptr->tm_min / 10) % 10;
			tmp[1] +=  timeptr->tm_min % 10;
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
